/*
 *         algo0x0e.cpp SpliX is Copyright 2006-2008 by Aurélien Croc
 *                      This file is a SpliX derivative work
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
 * $Id$
 *
 * --
 * This code is written by Leonardo Hamada
 */
#include "algo0x0e.h"
#include <vector>
#include <span>
#include <memory>
#include <cstdint>
#include <algorithm>
#include "errlog.h"
#include "request.h"
#include "printer.h"
#include "bandplane.h"

// Replaced macros with direct calls or inline logic



// LIFECYCLE: Managed by compiler defaults in the header.

inline void Algo0x0E::addLiteralSequence(
                                        std::vector<uint8_t> &output,
                                        std::span<const uint8_t> data,
                                        uint32_t position,
                                        uint32_t length,
                                        uint32_t blanks )
{
    /* Set control value for the literal chunk length. */
    uint32_t tmp = length + blanks - 1;

    output.push_back(0x80 | static_cast<uint8_t>(tmp >> 8));
    output.push_back(static_cast<uint8_t>(tmp));

    /* Copy literal data chunk. */
    if (length > 0) {
        auto literalSource = data.subspan(position, length);
        output.insert(output.end(), literalSource.begin(), literalSource.end());
    }

    /* Pad with required blanks. */
    if (blanks > 0) {
        output.insert(output.end(), blanks, 0xff);
    }
}

inline void Algo0x0E::addReplicativeRun(
                                       std::vector<uint8_t> &output,
                                       uint32_t runs,
                                       uint8_t value )
{
    /* Verify run numbers to choose appropriate control header. */
    if ( 65 >= runs ) {
        output.push_back(0x7f & static_cast<uint8_t>(1L - runs));
    } else {
        uint32_t t = 0xffff - runs + 2;

        output.push_back(static_cast<uint8_t>(t >> 8));
        output.push_back(static_cast<uint8_t>(t));
    }

    /* Set value to be replicated. */
    output.push_back(value);
}



/* 
 * Check if the segment starting at the beginning of 'data' can be encoded 
 * as consecutive runs with a positive encoding gain.
 * Returns the gain in bytes.
 */
uint32_t Algo0x0E::verifyGain(std::span<const uint8_t> data)
{
    uint32_t gain = 0;
    size_t e = 0;
    const size_t L = data.size();

    while (e + 1 < L) {
        uint32_t runLength = 1;
        while (e + runLength < L && data[e + runLength - 1] == data[e + runLength]) {
            if (++runLength == 4) { // Heuristic: cap at 4 for gain verify
                break;
            }
        }
        
        if (runLength >= 2) {
            gain += (runLength <= 65) ? (runLength - 2) : (runLength - 3);
            if (gain >= 2) {
                return gain;
            }
            e += runLength;
        } else {
            break;
        }
    }
    return gain;
}

/*
 * Returns the new 'q' index after encoding as many replicative runs as possible.
 */
uint32_t Algo0x0E::encodeReplications(uint32_t q, uint32_t L,
                                          std::span<const uint8_t> data,
                                          std::vector<uint8_t> &output)
{
    while (q < L) {
        uint32_t r = 1;
        while (q + r < L && data[q + r - 1] == data[q + r]) {
            r++;
        }
        if (r >= 2) {
            addReplicativeRun(output, r, data[q]);
            q += r;
        } else {
            break;
        }
    }
    return q;
}

/*
 * Seeks the beginning position of the last segment of the scan-line 
 * that can be encoded as contiguous replication runs.
 */
uint32_t Algo0x0E::locateBackwardReplications(std::span<const uint8_t> data)
{
    if (data.empty()) return 0;

    int32_t i = static_cast<int32_t>(data.size()) - 1;     
    while (i > 0) {
        uint32_t r = 1;
        while (i - static_cast<int32_t>(r) >= 0 && data[i - r + 1] == data[i - r]) {
            r++;
        }
        if (r > 1) {
            i = i - r;
        } else {
            break;
        }
    }
    return static_cast<uint32_t>(i + 1);
}

SP::Result<std::unique_ptr<BandPlane>> Algo0x0E::compress([[maybe_unused]] const Request & request, std::span<const uint8_t> data,
                               uint32_t width, uint32_t height)
{
    /* Basic parameters validation. */
    if ( data.empty() || !height || !width ) {
        ERRORMSG(_("Invalid given data for compression: 0xe"));
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    /* Input sanitization: prevent absurd dimensions. */
    if (width > 65536 || height > 65536) {
        ERRORMSG(_("Absurd dimensions for compression: %u x %u"), width, height);
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    /* We will interpret the band height of 128 pixels as 600 DPI printing
       request. Likewise, height of 64 pixels as 300 DPI printing. */
    if ( ! ( 128 == height || 64 == height ) ) {
        ERRORMSG(_("Invalid band height for compression: 0xe"));
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    /* The row-bytes of the bitmap. */
    const uint32_t rowBytes = ( width + 7 ) / 8;

    /* This is the allowed raw data size per scan-line. */
    const uint32_t maxWorkRb = ( 0x40 == height ) ?
        0x09A0 / 8 : 0x1360 / 8;

    /* If rowBytes is larger than allowed size, print will be cropped. */
    const uint32_t workRb = ( rowBytes > maxWorkRb ) ?
        maxWorkRb : rowBytes;

    /* Pre-calculate maximum possible output size and verify safety limits. */
    const uint32_t estimatedOutSize = ( 2 + maxWorkRb ) * height + 3;
    if (estimatedOutSize > 512 * 1024 * 1024) {
        ERRORMSG(_("Compression buffer size exceeds safety limit: %u"), estimatedOutSize);
        return SP::Unexpected(SP::Error::MemoryError);
    }

    std::vector<uint8_t> output;
    try {
        /* Estimate a buffer size equal to 2-byte control header overhead +
           maxWorkRb, times the bitmap height + up to 3-byte padding at end. */
        output.reserve(estimatedOutSize);
    } catch( const std::bad_alloc & ){
        /* Catch error if buffer creation fails. */
        ERRORMSG(_("Could not allocate work buffer for encoding: 0xe"));
        return SP::Unexpected(SP::Error::MemoryError);
    }

    /* Main encoding loop for each scan-line.
       Top to bottom scan-line processing. */
    for (uint32_t h = height; h > 0; --h) {
        if (data.empty()) break; // Safety

        // Current scanline view
        std::span<const uint8_t> currentData = data.subspan(0, std::min<size_t>(workRb, data.size()));

        /* Adjust this working scan-line size
           up to where there is no blank bytes on the right end. */
        auto it = std::find_if(currentData.rbegin(), currentData.rend(), 
                               [](uint8_t b) { return b != 0xff; });
        
        uint32_t E = static_cast<uint32_t>(std::distance(it, currentData.rend()));

        /* Determine the number of padding blank bytes to the right
           end of the scan-line relative to constant max. width. */ 
        uint32_t B = maxWorkRb - E;
        
        /* If scan-line is not blank and the number of blank padding is 
           not 1, seek the beginning position of the last scan-line segment,
           when it can be encoded as contiguous replication runs. */
        uint32_t F = ((E > 0) && (B != 1)) ? locateBackwardReplications(currentData.subspan(0, E)) : E;
  
        /* Try to encode the first segment as replication runs. */
        uint32_t i = (F > 0) ? encodeReplications(0, F, currentData, output) : 0;

        /* Continue to encode the rest of data as replication or literal
           segment chunks as appropriate. */
        /* l: length of cumulative literal segment.*/
        uint32_t l = 0;
        while (i + l + 1 < F) {
            if (currentData[i + l + 1] != currentData[i + l]) {
                l++;
            } else {
                if (verifyGain(currentData.subspan(i + l, F - (i + l))) >= 2) {
                    addLiteralSequence(output, currentData, i, l, 0);
                    i = encodeReplications(i + l, F, currentData, output);
                    l = 0;                                
                } else {
                    l++;
                }
            }
        }
        
        /* Encode last remaining segments and white paddings. */  
        if (F < E) {
            /* Encode previous literal segment that remains unencoded. */
            if (i < F) {
                addLiteralSequence(output, currentData, i, F - i, 0);
            }
            /* Encode the last non blank replication runs. */
            i = encodeReplications(F, E, currentData, output);
            /* Ends a scan-line with required white paddings */
            if (B >= 2) {
                addReplicativeRun(output, B, 0xff);
            }
        } else {
            /* When the end of non blank data of a scan-line does not 
               end with replication runs, encode with literal sequence. */  
            if (B >= 2) {
                /* Encode previous literal segment that remains unencoded. */
                if (i < E) {
                    addLiteralSequence(output, currentData, i, E - i, 0);
                }
                /* Ends a scan-line with required white paddings. */
                addReplicativeRun(output, B, 0xff);
            } else if ((B > 0) || (i < E)) {
                /* Ends a scan-line encoding with a 
                   literal sequence with blanks if any. */
                addLiteralSequence(output, currentData, i, E - i, B);
            }
        }

        if (h > 1) {
            /* Proceed to the next scan-line. */
            data = data.subspan(std::min<size_t>(rowBytes, data.size()));
        }
    }

    /* Zero value byte padding for data size alignment to 4-bytes bounds. */
    uint32_t zerosPad = 4 - (static_cast<uint32_t>(output.size()) % 4);

    /* Check if it is already aligned. */
    if (zerosPad > 0 && zerosPad < 4) {
        output.insert(output.end(), zerosPad, 0);
    }

    /* Prepare to return data encoded by algorithm 0xe. */
    auto plane = std::make_unique<BandPlane>();

    /* Register data and its size. */
    uint32_t finalSize = static_cast<uint32_t>(output.size());
    plane->setData(std::move(output));
    plane->setEndian(BandPlane::Endian::Dependant);

    /* Set this band encoding type. */
    plane->setCompression(0xe);
    DEBUGMSG(_("Finished band encoding: type=0xe, size=%u"), finalSize);
    return plane;
}

