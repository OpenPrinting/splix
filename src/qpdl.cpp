/*
 * 	    qpdl.cpp                  (C) 2006-2008, Aurélien Croc (AP²C)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the
 *  Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  $Id$
 * 
 */
#include "qpdl.h"
#include "sp_result.h"
#include <errno.h>
#include "sp_portable.h"
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include "page.h"
#include "band.h"
#include "errlog.h"
#include "request.h"
#include "bandplane.h"
#include <array>
#include <span>
#include <vector>

/* Helper for safe writing to STDOUT with result propagation */
static SP::Result<> _safeWrite(std::span<const unsigned char> data)
{
    while (!data.empty()) {
        ssize_t written = write(SP_STDOUT_FILENO, data.data(), data.size());
        if (written == -1) {
            if (errno == EINTR) continue;
            ERRORMSG(_("Error while sending data to the printer (%d)"), errno);
            return SP::Unexpected(SP::Error::IOError);
        }
        data = data.subspan(written);
    }
    return {};
}

/* Support function for algorithm of type 0x15 printers. */
static SP::Result<> _outputAuxRecords(const Page* page)
{
    // Get the first plane containing plane data.
    const Band *band = page->firstBand();
    const std::array<unsigned char, 16> header = { 
        0x13, 0, 0, 0, 0x23, 0x15, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0x14 
    };
    if (!band)
        return {};

    // Output record type 0x13 and marker for record 0x14 .
    if (auto res = _safeWrite(header); !res) return res;

    // Output BIH of JBIG data.
    if (page->getBIH()) {
        if (auto res = _safeWrite(std::span(reinterpret_cast<const unsigned char*>(page->getBIH()), 20)); !res) return res;
    } else {
        ERRORMSG(_("Error getting BIH data for page"));
        return SP::Unexpected(SP::Error::InconsistentData);
    }
    std::array<unsigned char, 4> trail;
    trail[0] = 0; trail[1] = 0; trail[2] = 1;
    trail[3] = static_cast<unsigned char>((band->width() >> 8) + 65);
    return _safeWrite(trail);
}

static SP::Result<> _renderJBIGBand([[maybe_unused]] const Request& request, const Band* band, bool mono)
{
    std::array<unsigned char, 0xc> header;
    // Black=4, Cyan=1, Magenta=2, Yellow=3
    int color_order[ 4 ] = { 4, 1, 2, 3 };
    int colorsNr = mono ? 1:4;
    unsigned long dataSize, checkSum, lineBytes;
    const BandPlane *plane = nullptr;
    // Cycle through each color planes 
    for ( int j = 0; j < colorsNr; j++ ) {
        int current_color = color_order[ j ];
        // Search in the current band, a plane of current color.
        plane = nullptr;
        for (const BandPlane* search_plane : band->planes()) {
            if (search_plane && current_color == search_plane->colorNr()) {
                plane = search_plane;
                break;
            }
        }
        // Continue to the next color if no data are present.
        if (!plane) 
            continue;
        // Output record of type 0xC.
        header[0x0] = 0xC;                      // Signature
        header[0x1] = static_cast<unsigned char>(band->bandNr());           // Band number
        // Compute the bytes per line of the pixel data.
        lineBytes = (band->width() + 7) / 8;
        header[0x2] = static_cast<unsigned char>(lineBytes >> 8);           // Band width 8-15
        header[0x3] = static_cast<unsigned char>(lineBytes);                // Band width 0-7
        header[0x4] = static_cast<unsigned char>(band->height() >> 8);      // Band height 8-15
        header[0x5] = static_cast<unsigned char>(band->height());           // Band height 0-7
        header[0x6] = static_cast<unsigned char>(current_color);            // Color number
        header[0x7] = static_cast<unsigned char>(plane->compression());     // Compression algorithm 0x15
        dataSize = static_cast<uint32_t>(plane->dataSize() + 4);
        // Append the last information and send the header
        header[0x8] = static_cast<unsigned char>(dataSize >> 24);            // Data size 24 - 31
        header[0x9] = static_cast<unsigned char>(dataSize >> 16);            // Data size 16 - 23
        header[0xa] = static_cast<unsigned char>(dataSize >> 8);             // Data size 8 - 15
        header[0xb] = static_cast<unsigned char>(dataSize);                  // Data size 0 - 7

        if (auto res = _safeWrite(std::span(header.data(), 0xC)); !res) return res;

        // Send the data
        if (auto res = _safeWrite(plane->data_span()); !res) return res;

        // Calculate and send the checksum
        checkSum  = plane->checksum();
        std::array<unsigned char, 4> cs;
        cs[0] = static_cast<unsigned char>(checkSum >> 24);              // Checksum 24 - 31
        cs[1] = static_cast<unsigned char>(checkSum >> 16);              // Checksum 16 - 23
        cs[2] = static_cast<unsigned char>(checkSum >> 8);               // Checksum 8 - 15
        cs[3] = static_cast<unsigned char>(checkSum);                    // Checksum 0 - 7

        if (auto res = _safeWrite(cs); !res) return res;
    }
    return {};
}

static SP::Result<> _renderBand(const Request& request, const Band* band, bool mono)
{
    unsigned long version, subVersion, size, dataSize, checkSum;
    bool color, headerSent=false;
    std::array<unsigned char, 0x20> header;
    const BandPlane *plane;

    version = request.printer()->qpdlVersion();
    color = request.printer()->color();
    subVersion = band->parent()->compression() == 0x13 ? 3 : 0;

    size_t planeIdx = 0;
    for (const BandPlane* plane : band->planes()) {
        unsigned long compression;
        bool nextBand = false;

        if (!plane) {
            ERRORMSG(_("Inconsistent data. Operation aborted"));
            return SP::Unexpected(SP::Error::InconsistentData);
        }
        compression = plane->compression();
        checkSum = plane->checksum();

        // Check if there is a next band for that color
        if (subVersion) {
            const BandPlane *nextPlane;
            const Band *next;

            next = band->sibling();
            if (next) {
                for (unsigned int j=0; j < next->planesNr(); j++) {
                    nextPlane = next->plane(j);
                    if (nextPlane && nextPlane->colorNr() == plane->colorNr()){
                        nextBand = true;
                        break;
                    }
                }
            }
        }

        // Calculate the data size
        dataSize = static_cast<unsigned long>(plane->data_span().size());
        if (compression != 0x0D && compression != 0x0E)
            dataSize += 4;              // Data signature
        if (version > 0) {
            dataSize += 4;              // Checksum
            if (subVersion == 3)
                dataSize += 7*4;        // Sub-header
        }

        // Send the header
        if (!headerSent || version == 2) {
            header[0x0] = 0xC;                      // Signature
            header[0x1] = static_cast<unsigned char>(band->bandNr());           // Band number
            header[0x2] = static_cast<unsigned char>(band->width() >> 8);       // Band width 8-15
            header[0x3] = static_cast<unsigned char>(band->width());            // Band width 0-7
            header[0x4] = static_cast<unsigned char>(band->height() >> 8);      // Band height 8-15
            header[0x5] = static_cast<unsigned char>(band->height());           // Band height 0-7
            headerSent = true;
            size = 0x6;
        } else
            size = 0x0;
        // Add color information if it's a color printer
        if (color) {
            header[size] = static_cast<unsigned char>(mono ? 4 : plane->colorNr()); // Color number
            size++;
        }
        // Append the last information and send the header
        header[size+0] = static_cast<unsigned char>(compression);               // Compression algorithm
        header[size+1] = static_cast<unsigned char>(dataSize >> 24);            // Data size 24 - 31
        header[size+2] = static_cast<unsigned char>(dataSize >> 16);            // Data size 16 - 23
        header[size+3] = static_cast<unsigned char>(dataSize >> 8);             // Data size 8 - 15
        header[size+4] = static_cast<unsigned char>(dataSize);                  // Data size 0 - 7
        if (auto res = _safeWrite(std::span(header.data(), size+5)); !res) return res;

        // Send the sub-header
        if (compression != 0x0D && compression != 0x0E) {
            switch (plane->endian()) {
                case BandPlane::Endian::Dependant: {
                    // Machine-native byte order — must match original uint32_t* cast behavior
                    uint32_t sig = static_cast<uint32_t>(0x09ABCDEF + (subVersion << 28));
                    memcpy(&header[0x0], &sig, sizeof(sig));
                    break;
                }
                case BandPlane::Endian::BigEndian:
                    header[0x0] = static_cast<unsigned char>(0x9 + (subVersion << 0x4));
                                                        // Sub-header signature1
                    header[0x1] = 0xAB;                 // Sub-header signature2
                    header[0x2] = 0xCD;                 // Sub-header signature3
                    header[0x3] = 0xEF;                 // Sub-header signature4
                    break;
                case BandPlane::Endian::LittleEndian:
                    header[0x0] = 0xEF;                 // Sub-header signature4
                    header[0x1] = 0xCD;                 // Sub-header signature3
                    header[0x2] = 0xAB;                 // Sub-header signature2
                    header[0x3] = static_cast<unsigned char>(0x9 + (subVersion << 0x4));
                                                        // Sub-header signature1
                    break;
            };
            size = 4;
            if (subVersion == 3) {
                uint32_t state;
                auto pSpan = plane->data_span();
                size_t dSize = pSpan.size();

                checkSum += 0x39 + 0xAB + 0xCD + 0xEF;
                if (!band->bandNr())
                    state = 0x0;                        // First band
                else if (nextBand) {
                    state = 0x01000000;                 // Next band available
                    checkSum += 0x01;
                } else {
                    state = 0x02000000;                 // Last band
                    checkSum += 0x02;
                }
                memset(header.data() + size + 4, 0, 6 * 4);

                switch (plane->endian()) {
                    case BandPlane::Endian::Dependant: {
                        // Machine-native byte order — must match original uint32_t* cast behavior
                        uint32_t udSize = static_cast<uint32_t>(dSize);
                        memcpy(&header[size], &udSize, sizeof(udSize));
                        memcpy(&header[size + 4], &state, sizeof(state));
                        break;
                    }
                    case BandPlane::Endian::BigEndian:
                        // Data size 24 - 31
                        header[size+0] = static_cast<unsigned char>(dSize >> 24);
                        // Data size 16 - 23
                        header[size+1] = static_cast<unsigned char>(dSize >> 16);
                        // Data size 8 - 15
                        header[size+2] = static_cast<unsigned char>(dSize >> 8);
                        // Data size 0 - 7
                        header[size+3] = static_cast<unsigned char>(dSize);
                        // State 24 - 31
                        header[size+4] = static_cast<unsigned char>(state >> 24);
                        // State 16 - 23
                        header[size+5] = static_cast<unsigned char>(state >> 16);
                        // State 8 - 15
                        header[size+6] = static_cast<unsigned char>(state >> 8);
                        // State 0 - 7
                        header[size+7] = static_cast<unsigned char>(state);
                        break;
                    case BandPlane::Endian::LittleEndian:
                        // Data size 0 - 7
                        header[size+0] = static_cast<unsigned char>(dSize);
                        // Data size 8 - 15
                        header[size+1] = static_cast<unsigned char>(dSize >> 8);
                        // Data size 16 - 23
                        header[size+2] = static_cast<unsigned char>(dSize >> 16);
                        // Data size 24 - 31
                        header[size+3] = static_cast<unsigned char>(dSize >> 24);
                        // State 0 - 7
                        header[size+4] = static_cast<unsigned char>(state);
                        // State 8 - 15
                        header[size+5] = static_cast<unsigned char>(state >> 8);
                        // State 16 - 23
                        header[size+6] = static_cast<unsigned char>(state >> 16);
                        // State 24 - 31
                        header[size+7] = static_cast<unsigned char>(state >> 24);
                        break;
                }
                for (unsigned int j=0; j < 4; j++)
                    checkSum += header[size + j];
                size += 4 + 4 + 5*4;
            } else
                for (unsigned int j=0; j < 4; j++)
                    checkSum += header[size - j - 1];
            if (auto res = _safeWrite(std::span(header.data(), size)); !res) return res;
        }
        
        // Send the data
        if (auto res = _safeWrite(plane->data_span()); !res) return res;

        // Send the checksum
        header[0] = static_cast<unsigned char>(checkSum >> 24);                 // Checksum 24 - 31
        header[1] = static_cast<unsigned char>(checkSum >> 16);                 // Checksum 16 - 23
        header[2] = static_cast<unsigned char>(checkSum >> 8);                  // Checksum 8 - 15
        header[3] = static_cast<unsigned char>(checkSum);                       // Checksum 0 - 7
        size = 4;
            // Close the plane if needed
        if (color && (version == 1 || version == 5) && 
            (planeIdx + 1) == band->planesNr()) {
            header[4] = 0;
            size++;
        }
        if (auto res = _safeWrite(std::span(header.data(), size)); !res) return res;
        planeIdx++;
    }

    return {};
}

SP::Result<> renderPage(const Request& request, Page* page, bool lastPage)
{
    unsigned char duplex=0, tumble=0, paperSource=1;
    unsigned long width, height;
    std::array<unsigned char, 0x11> header;
    const Band* band;
    SP::Result<> (*selectedRenderBand)(const Request&, const Band*, bool);
    

    if (!page) {
        ERRORMSG(_("Try to render a NULL page"));
        return SP::Unexpected(SP::Error::InconsistentData);
    }

    // Get the duplex values
    paperSource = request.printer()->paperSource();
    switch (request.duplex()) {
        case Request::Simplex:
            /* Observed a value of 0 for 0x15 printers. */
            duplex = (0x15 == page->compression())? 0 : 1;
            tumble = 0;
            break;
        case Request::LongEdge:
            duplex = 1;
            tumble = page->pageNr() % 2;
            break;
        case Request::ShortEdge:
            duplex = 0;
            tumble = page->pageNr() % 2;
            break;
        case Request::ManualLongEdge:
            duplex = 0;
            tumble = page->pageNr() % 2;
            if (tumble && !lastPage)
                paperSource = 3; // Multi source
            break;
        case Request::ManualShortEdge:
            duplex = 0;
            tumble = page->pageNr() % 2;
            if (tumble && !lastPage)
                paperSource = 3; // Multi source
            /** @todo what about the Short edge? The page isn't rotated?  */
            break;
    }

    width = page->width();
    height = page->height();

    if (0x15 == page->compression()) {
        selectedRenderBand = &_renderJBIGBand;
    } else {
        selectedRenderBand = &_renderBand;
    }

    if (request.printer()->fixedBandWidth() || 0x15 == page->compression()) {
        // For CLP-310/315 and fixed bandwidth printers, multiply page
        // dimensions in inches by 300.
        width = static_cast<unsigned long>(ceil(300 * (request.printer()->pageWidth() / 72.0)));
        height = static_cast<unsigned long>(ceil(300 * (request.printer()->pageHeight() / 72.0)));
    }

    // Send the page header
    header[0x0] = 0;                                // Signature
    header[0x1] = static_cast<unsigned char>(page->yResolution() / 100);        // Y Resolution
    header[0x2] = static_cast<unsigned char>(page->copiesNr() >> 8);            // Number of copies 8-15
    header[0x3] = static_cast<unsigned char>(page->copiesNr());                 // Number of copies 0-7
    header[0x4] = static_cast<unsigned char>(request.printer()->paperType());   // Paper type
    header[0x5] = static_cast<unsigned char>(width >> 8);                       // Printable area width
    header[0x6] = static_cast<unsigned char>(width);                            // Printable area width
    header[0x7] = static_cast<unsigned char>(height >> 8);                      // Printable area height
    header[0x8] = static_cast<unsigned char>(height);                           // Printable area height
    header[0x9] = static_cast<unsigned char>(paperSource);                      // Paper source
    header[0xa] = static_cast<unsigned char>(request.printer()->unknownByte1());// Always 0 in ULD
    header[0xb] = duplex;                           // Duplex
    header[0xc] = tumble;                           // Tumble
    header[0xd] = static_cast<unsigned char>(request.printer()->unknownByte2());// Always 0 in ULD
    header[0xe] = static_cast<unsigned char>(request.printer()->qpdlVersion()); // QPDL Version
    header[0xf] = static_cast<unsigned char>(request.printer()->unknownByte3());// Colorplanes
    header[0x10] = static_cast<unsigned char>(page->xResolution() / 100);       // X Resolution
    if (auto res = _safeWrite(std::span(header.data(), 0x11)); !res) return res;

    // Send auxiliary records for clp-315 printers.
    if (0x15 == page->compression()) {
        auto auxRes = _outputAuxRecords(page);
        if (!auxRes)
            return auxRes;
    }

    // Send the page bands
    band = page->firstBand();
    while (band) {
        auto res = selectedRenderBand(request, band, page->colorsNr() == 1);
        if (!res)
            return res;
        band = band->sibling();
    }

    // Send the page footer
    header[0x0] = 1;                                // Signature
    header[0x1] = static_cast<unsigned char>(page->copiesNr() >> 8);            // Number of copies 8-15
    header[0x2] = static_cast<unsigned char>(page->copiesNr());                 // Number of copies 0-7
    if (auto res = _safeWrite(std::span(header.data(), 0x3)); !res) return res;

    return {};
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

