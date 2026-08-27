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
        const auto literalSource = data.subspan(position, length);
        for (const uint8_t value : literalSource)
            output.push_back(value);
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
