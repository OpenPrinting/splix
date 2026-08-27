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
