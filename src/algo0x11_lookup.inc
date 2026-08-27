/*
 * 	    algo0x11.cpp              (C) 2006-2008, Aurélien Croc (AP²C)
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
#include "algo0x11.h"
#include <cstring>
#include <cstdlib>
#include <vector>
#include <span>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <ranges>
#include "bandplane.h"
#include "errlog.h"



/*
 * Fonctions locales
 * Local functions
 */
// Removed legacy __compare function

SP::Result<> Algo0x11::_lookupBestOccurs(std::span<const uint8_t> data)
{
    struct Occurence {
        uint32_t count;
        uint32_t offset;
    };

    // Safety check: Don't process if data is too small for sampling
    if (data.size() < Algo0x11::COMPRESS_SAMPLE_RATE) {
        return {}; // Not an error, just can't optimize
    }

    try {
        std::vector<Occurence> occurs(Algo0x11::COMPRESS_SAMPLE_RATE);
        bool oneIsPresent = false;
        uint32_t size = static_cast<uint32_t>(data.size());

        // Initialize the table
        for (uint32_t i=0; i < Algo0x11::COMPRESS_SAMPLE_RATE; i++) {
            occurs[i] = {0, i};
        }

        // Calculate the byte occurrence
        for (uint32_t i=Algo0x11::COMPRESS_SAMPLE_RATE; i < size; i += Algo0x11::COMPRESS_SAMPLE_RATE) {
            uint8_t b = data[i];
            for (uint32_t j=1; j < Algo0x11::COMPRESS_SAMPLE_RATE; j++) {
                if (data[i - j] == b) {
                    occurs[j - 1].count++;
                }
            }
        }

        // Order the array (biggest first)
        std::ranges::sort(occurs, [](const Occurence& a, const Occurence& b) {
            return a.count > b.count;
        });

        // Set the pointer table to use for compression
        for (uint32_t i=0; i < Algo0x11::TABLE_PTR_SIZE; i++) {
            _ptrArray[i] = occurs[i].offset + 1;
            if (_ptrArray[i] == 1) {
                oneIsPresent = true;
            }
        }
        // Append the value 1 which improves the compression of multiple same bytes
        if (!oneIsPresent) {
            _ptrArray[Algo0x11::TABLE_PTR_SIZE-1] = 1;
        }
    } catch (const std::bad_alloc&) {
        return SP::Unexpected(SP::Error::MemoryError);
    }

    return {};
}
