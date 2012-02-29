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
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include "page.h"
#include "band.h"
#include "errlog.h"
#include "request.h"
#include "bandplane.h"

/* Support function for algorithm of type 0x15 printers. */
static bool _outputAuxRecords(const Page* page)
{
    // Get the first plane containg plane data.
    const Band *band = page->firstBand();
    unsigned char header[16] = { 0x13, 0, 0, 0, 0x23, 0x15, 0, 0,
                                    0, 0, 0, 0, 0, 0, 0, 0x14 };
    if (!band)
        return true;
    // Output record type 0x13 and marker for record 0x14 .
    if (write(STDOUT_FILENO, (unsigned char*)&header, 16) == -1) {
        ERRORMSG(_("Error while sending data to the printer (%u)"), errno);
        return false;
    }
    // Output BIH of JBIG data.
    if (page->getBIH()) {
        if (write(STDOUT_FILENO, page->getBIH(), 20) == -1) {
            ERRORMSG(_("Error while sending data to the printer (%u)"), errno);
            return false;
        }
    } else {
        ERRORMSG(_("Error getting BIH data for page (%u)"), errno);
        return false;
    }
    header[0] = 0; header[1] = 0; header[2] = 1;
    header[3] = (band->width() >> 8) + 65;
    if (write(STDOUT_FILENO, (unsigned char*)&header, 4) == -1) {
        ERRORMSG(_("Error while sending data to the printer (%u)"), errno);
        return false;
    }
    return true;
}

static bool _renderJBIGBand(const Request& request, const Band* band, bool mono)
{
    unsigned char header[0xc];
    // Black=4, Cyan=1, Magenta=2, Yellow=3
    int color_order[ 4 ] = { 4, 1, 2, 3 };
    int colorsNr = mono ? 1:4;
    unsigned long dataSize, checkSum, lineBytes;
    const BandPlane *plane = NULL;
    // Cycle through each color planes 
    for ( int j = 0; j < colorsNr; j++ ) {
        int current_color = color_order[ j ];
        // Search in the current band, a plane of current color.
        plane = NULL;
        for (unsigned int i = 0; i < band->planesNr(); i++) {
            const BandPlane * search_plane = band->plane(i);
            if (!search_plane)
                continue;
            if (current_color == search_plane->colorNr()) {
                plane = search_plane;
                break;
            }
        }
        // Continue to the next color if no data are present.
        if (!plane) 
            continue;
        // Output record of type 0xC.
        header[0x0] = 0xC;                      // Signature
        header[0x1] = band->bandNr();           // Band number
        // Compute the bytes per line of the pixel data.
        lineBytes = (band->width() + 7) / 8;
        header[0x2] = lineBytes >> 8;           // Band width 8-15
        header[0x3] = lineBytes;                // Band width 0-7
        header[0x4] = band->height() >> 8;      // Band height 8-15
        header[0x5] = band->height();           // Band height 0-7
        header[0x6] = current_color;            // Color number
        header[0x7] = plane->compression();     // Compression algorithm 0x15
        dataSize = plane->dataSize() + 4;
        // Append the last information and send the header
        header[0x8] = dataSize >> 24;            // Data size 24 - 31
        header[0x9] = dataSize >> 16;            // Data size 16 - 23
        header[0xa] = dataSize >> 8;             // Data size 8 - 15
        header[0xb] = dataSize;                  // Data size 0 - 7
        if (write(STDOUT_FILENO, (unsigned char*)&header, 0xc) == -1) {
            ERRORMSG(_("Error while sending data (%u)"), errno);
            return false;
        }
        // Send the data
        if (write(STDOUT_FILENO, plane->data(), plane->dataSize()) == -1) {
            ERRORMSG(_("Error while sending data (%u)"), errno);
            return false;
        }
        // Calculate and send the checksum
        checkSum  = plane->checksum();
        header[0] = checkSum >> 24;              // Checksum 24 - 31
        header[1] = checkSum >> 16;              // Checksum 16 - 23
        header[2] = checkSum >> 8;               // Checksum 8 - 15
        header[3] = checkSum;                    // Checksum 0 - 7
        if (write(STDOUT_FILENO, (unsigned char*)&header, 4) == -1) {
            ERRORMSG(_("Error while sending data (%u)"), errno);
            return false;
        }
    }
    return true;
}

static bool _renderBand(const Request& request, const Band* band, bool mono)
{
    unsigned long version, subVersion, size, dataSize, checkSum;
    bool color, headerSent=false;
    unsigned char header[0x20];
    const BandPlane *plane;

    version = request.printer()->qpdlVersion();
    color = request.printer()->color();
    subVersion = band->parent()->compression() == 0x13 ? 3 : 0;

    for (unsigned int i=0; i < band->planesNr(); i++) {
        unsigned long compression;
        bool nextBand = false;

        // Get the plane
        plane = band->plane(i);
        if (!plane) {
            ERRORMSG(_("Inconsistent data. Operation aborted"));
            return false;
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
        dataSize = plane->dataSize();
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
            header[0x1] = band->bandNr();           // Band number
            header[0x2] = band->width() >> 8;       // Band width 8-15
            header[0x3] = band->width();            // Band width 0-7
            header[0x4] = band->height() >> 8;      // Band height 8-15
            header[0x5] = band->height();           // Band height 0-7
            headerSent = true;
            size = 0x6;
        } else
            size = 0x0;
        // Add color information if it's a color printer
        if (color) {
            header[size] = mono ? 4 : plane->colorNr(); // Color number
            size++;
        }
        // Append the last information and send the header
        header[size+0] = compression;               // Compression algorithm
        header[size+1] = dataSize >> 24;            // Data size 24 - 31
        header[size+2] = dataSize >> 16;            // Data size 16 - 23
        header[size+3] = dataSize >> 8;             // Data size 8 - 15
        header[size+4] = dataSize;                  // Data size 0 - 7
        if (write(STDOUT_FILENO, (unsigned char*)&header, size+5) == -1) {
            ERRORMSG(_("Error while sending data to the printer (%u)"), errno);
            return false;
        }

        // Send the sub-header
        if (compression != 0x0D && compression != 0x0E) {
            switch (plane->endian()) {
                case BandPlane::Dependant:
                    *(uint32_t *)&header = (uint32_t)(0x09ABCDEF + 
                            (subVersion << 28));        // Sub-header signature
                    break;
                case BandPlane::BigEndian:
                    header[0x0] = 0x9 + (subVersion << 0x4);
                                                        // Sub-header signature1
                    header[0x1] = 0xAB;                 // Sub-header signature2
                    header[0x2] = 0xCD;                 // Sub-header signature3
                    header[0x3] = 0xEF;                 // Sub-header signature4
                    break;
                case BandPlane::LittleEndian:
                    header[0x0] = 0xEF;                 // Sub-header signature4
                    header[0x1] = 0xCD;                 // Sub-header signature3
                    header[0x2] = 0xAB;                 // Sub-header signature2
                    header[0x3] = 0x9 + (subVersion << 0x4);
                                                        // Sub-header signature1
                    break;
            };
            size = 4;
            if (subVersion == 3) {
                uint32_t state;

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
                memset(header + size + 4, 0, 6*4);

                switch (plane->endian()) {
                    case BandPlane::Dependant:
                        *(uint32_t*)(&header + size) = (uint32_t)plane->
                            dataSize();
                        *(uint32_t*)(&header + size + 4) = (uint32_t)state;
                        break;
                    case BandPlane::BigEndian:
                        // Data size 24 - 31
                        header[size+0] = plane->dataSize() >> 24;
                        // Data size 16 - 23
                        header[size+1] = plane->dataSize() >> 16;
                        // Data size 8 - 15
                        header[size+2] = plane->dataSize() >> 8;
                        // Data size 0 - 7
                        header[size+3] = plane->dataSize();
                        // State 24 - 31
                        header[size+4] = state >> 24;
                        // State 16 - 23
                        header[size+5] = state >> 16;
                        // State 8 - 15
                        header[size+6] = state >> 8;
                        // State 0 - 7
                        header[size+7] = state;
                        break;
                    case BandPlane::LittleEndian:
                        // Data size 0 - 7
                        header[size+0] = plane->dataSize();
                        // Data size 8 - 15
                        header[size+1] = plane->dataSize() >> 8;
                        // Data size 16 - 23
                        header[size+2] = plane->dataSize() >> 16;
                        // Data size 24 - 31
                        header[size+3] = plane->dataSize() >> 24;
                        // State 0 - 7
                        header[size+4] = state;
                        // State 8 - 15
                        header[size+5] = state >> 8;
                        // State 16 - 23
                        header[size+6] = state >> 16;
                        // State 24 - 31
                        header[size+7] = state >> 24;
                        break;
                }
                for (unsigned int j=0; j < 4; j++)
                    checkSum += header[size + j];
                size += 4 + 4 + 5*4;
            } else
                for (unsigned int j=0; j < 4; j++)
                    checkSum += header[size - j - 1];
            if (write(STDOUT_FILENO, (unsigned char*)&header, size) == -1) {
                ERRORMSG(_("Error while sending data to the printer (%u)"),
                    errno);
                return false;
            }
        }
        
        // Send the data
        if (write(STDOUT_FILENO, plane->data(), plane->dataSize()) == -1) {
            ERRORMSG(_("Error while sending data to the printer (%u)"), errno);
            return false;
        }

        // Send the checksum
        header[0] = checkSum >> 24;                 // Checksum 24 - 31
        header[1] = checkSum >> 16;                 // Checksum 16 - 23
        header[2] = checkSum >> 8;                  // Checksum 8 - 15
        header[3] = checkSum;                       // Checksum 0 - 7
        size = 4;
            // Close the plane if needed
        if (color && (version == 1 || version == 5) && 
            (i+1) == band->planesNr()) {
            header[4] = 0;
            size++;
        }
        if (write(STDOUT_FILENO, (unsigned char*)&header, size) == -1) {
            ERRORMSG(_("Error while sending data to the printer (%u)"), errno);
            return false;
        }
    }

    return true;
}

bool renderPage(const Request& request, Page* page, bool lastPage)
{
    unsigned char duplex=0, tumble=0, paperSource=1;
    unsigned long width, height;
    unsigned char header[0x11];
    const Band* band;
    bool (*selectedRenderBand)(const Request&, const Band*, bool);
    

    if (!page) {
        ERRORMSG(_("Try to render a NULL page"));
        return false;
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
    // For CLP-310/315 printers, multiply page dimensions in inches by 300.
    // Also selects the appropriate band render function.
    if (0x15 == page->compression()) {
        width = ceil(300 * (request.printer()->pageWidth() / 72.0));
        height = ceil(300 * (request.printer()->pageHeight() / 72.0));
        selectedRenderBand = &_renderJBIGBand;
    } else {
        width = page->width();
        height = page->height();
        selectedRenderBand = &_renderBand;
    }
    // Send the page header
    header[0x0] = 0;                                // Signature
    header[0x1] = page->yResolution() / 100;        // Y Resolution
    header[0x2] = page->copiesNr() >> 8;            // Number of copies 8-15
    header[0x3] = page->copiesNr();                 // Number of copies 0-7
    header[0x4] = request.printer()->paperType();   // Paper type
    header[0x5] = width >> 8;                       // Printable area width
    header[0x6] = width;                            // Printable area width
    header[0x7] = height >> 8;                      // Printable area height
    header[0x8] = height;                           // Printable area height
    header[0x9] = paperSource;                      // Paper source
    header[0xa] = request.printer()->unknownByte1();// ??? XXX
    header[0xb] = duplex;                           // Duplex
    header[0xc] = tumble;                           // Tumble
    header[0xd] = request.printer()->unknownByte2();// ??? XXX
    header[0xe] = request.printer()->qpdlVersion(); // QPDL Version
    header[0xf] = request.printer()->unknownByte3();// ??? XXX
    header[0x10] = page->xResolution() / 100;       // X Resolution
    if (write(STDOUT_FILENO, (unsigned char*)&header, 0x11) == -1) {
        ERRORMSG(_("Error while sending data to the printer (%u)"), errno);
        return false;
    }

    // Send auxiliary records for clp-315 printers.
    if (0x15 == page->compression())
        if (!_outputAuxRecords(page))
            return false;

    // Send the page bands
    band = page->firstBand();
    while (band) {
        if (!selectedRenderBand(request, band, page->colorsNr() == 1))
            return false;
        band = band->sibling();
    }

    // Send the page footer
    header[0x0] = 1;                                // Signature
    header[0x1] = page->copiesNr() >> 8;            // Number of copies 8-15
    header[0x2] = page->copiesNr();                 // Number of copies 0-7
    if (write(STDOUT_FILENO, (unsigned char*)&header, 0x3) == -1) {
        ERRORMSG(_("Error while sending data to the printer (%u)"), errno);
        return false;
    }

    return true;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

