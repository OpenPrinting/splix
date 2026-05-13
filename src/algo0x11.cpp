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

SP::Result<> Algo0x11::_compress(std::span<const uint8_t> data, std::vector<uint8_t> &output)
{
    uint32_t r=0, uncompSize=0, maxCompSize, bestCompCounter, bestPtr;
    uint32_t rawDataCounter = 0, rawDataCounterPtr=0;
    uint32_t size = static_cast<uint32_t>(data.size());

    // Create the output buffer with estimated size
    output.clear();
    try {
        output.reserve(size + (Algo0x11::TABLE_PTR_SIZE * 2) + 4);
        output.resize(4); // placeholder for uncompSize
    } catch (const std::bad_alloc&) {
        return SP::Unexpected(SP::Error::MemoryError);
    }

    // Print the table
    for (uint32_t i=0; i < Algo0x11::TABLE_PTR_SIZE; i++) {
        uint16_t ptrVal = static_cast<uint16_t>(_ptrArray[i]);
        output.push_back(static_cast<uint8_t>(ptrVal & 0xFF));
        output.push_back(static_cast<uint8_t>((ptrVal >> 8) & 0xFF));
        if (_ptrArray[i] > uncompSize)
            uncompSize = _ptrArray[i];
    }

    // Print the first uncompressed bytes
    if (uncompSize > Algo0x11::MAX_UNCOMPRESSED_BYTES)
        uncompSize = Algo0x11::MAX_UNCOMPRESSED_BYTES;
    
    // Safety check: ensure r doesn't exceed data size
    if (uncompSize > size)
        uncompSize = size;

    // Correctly write uncompSize to the first 4 bytes
    for (int i=0; i<4; ++i) {
        output[i] = (uncompSize >> (i*8)) & 0xFF;
    }
    
    for (r=0; r < uncompSize; r++) {
        output.push_back(data[r]);
    }

    //
    // Compress the data
    //
    /* 6. Main compression loop. */
    do {
        const uint32_t remaining = size - r;
        const uint32_t maxCompSize = std::min<uint32_t>(remaining, Algo0x11::MAX_COMPRESSED_BYTES);

        // a) End of the compression?
        if (maxCompSize == 0) {
            if (rawDataCounter > 0) {
                output[rawDataCounterPtr] = static_cast<uint8_t>(rawDataCounter - 1);
            }
            break;
        } 
        
        // b) Try to find a match in the pointer table
        bool matchFound = false;
        if (maxCompSize >= 2) {
            uint32_t bestCompCounter = 0;
            uint32_t bestPtrIndex = 0;

            for (uint32_t i = 0; i < Algo0x11::TABLE_PTR_SIZE; i++) {
                const uint32_t offset = _ptrArray[i];
                if (offset > r) {
                    continue;
                }

                // Check how many bytes match at this offset
                const uint32_t rTmp = r - offset;
                uint32_t counter = 0;
                while (counter < maxCompSize && data[r + counter] == data[rTmp + counter]) {
                    counter++;
                }

                if (counter > bestCompCounter) {
                    bestCompCounter = counter;
                    bestPtrIndex = i;
                }
            }

            // If the match is large enough (e.g. > 1 byte saved), use it!
            if (bestCompCounter > Algo0x11::MIN_COMPRESSED_BYTES) {
                matchFound = true;
                
                // Finalize any pending raw data chunk
                if (rawDataCounter > 0) {
                    output[rawDataCounterPtr] = static_cast<uint8_t>(rawDataCounter - 1);
                    rawDataCounter = 0;
                }

                r += bestCompCounter;
                // Encoding format: flag bit | (count-adj)
                const uint32_t encodedCount = bestCompCounter - (Algo0x11::MIN_COMPRESSED_BYTES + 1);
                output.push_back(Algo0x11::COMPRESSION_FLAG | (encodedCount & 0x7F));
                output.push_back(((encodedCount >> 1) & 0xC0) | (bestPtrIndex & 0x3F));
            }
        }

        // c) If no match, write it as a literal (uncompressed) byte
        if (!matchFound) {
            if (rawDataCounter == 0) {
                rawDataCounterPtr = static_cast<uint32_t>(output.size());
                output.push_back(0); // Placeholder for count
            }

            output.push_back(data[r]);
            r++;
            rawDataCounter++;

            if (rawDataCounter == Algo0x11::MAX_UNCOMPRESSED_BYTES) {
                output[rawDataCounterPtr] = 0x7F; // Max literal flag
                rawDataCounter = 0;
            }
        }
    } while (r < size);

    // Safety check: if compressed output exceeds input size, compression has
    // expanded the data. The original driver used a fixed buffer of input size
    // and returned false on overflow. Match that behavior to protect printer
    // firmware decompressors that may have fixed receive buffers.
    if (output.size() >= data.size()) {
        ERRORMSG(_("No more space available in the output buffer for "
            "compression"));
        return SP::Unexpected(SP::Error::CompressionError);
    }

    return {};
}




// LIFECYCLE: Managed by compiler defaults in the header.



/*
 * Routine de compression
 * Compression routine
 */
SP::Result<std::unique_ptr<BandPlane>> Algo0x11::compress([[maybe_unused]] const Request& request, std::span<const uint8_t> data, 
        uint32_t width, uint32_t height)
{
    // Input sanitization
    if (width > 32768 || height > 32768 || width == 0 || height == 0) {
        ERRORMSG(_("Invalid raster dimensions for Algo0x11: %ux%u"), width, height);
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    uint64_t expectedSize = (static_cast<uint64_t>(width) * height) / 8;
    if (data.size() < expectedSize) {
        ERRORMSG(_("Input data too small for Algo0x11: got %zu, expected %llu"), data.size(), (unsigned long long)expectedSize);
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    // Safety limit on output size (512MB)
    if (expectedSize > 512 * 1024 * 1024) {
        ERRORMSG(_("Output size too large for security safety (0x11)"));
        return SP::Unexpected(SP::Error::MemoryError);
    }

    if (data.empty()) {
        ERRORMSG(_("Invalid given data for compression (0x11)"));
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    // Lookup for the best occurs
    auto lookupRes = _lookupBestOccurs(data);
    if (!lookupRes) {
        return SP::Unexpected(lookupRes.error());
    }
    
    std::vector<uint8_t> output;
    auto compressRes = _compress(data, output);
    if (!compressRes) {
        return SP::Unexpected(compressRes.error());
    }

    // Register the result into a band plane
    auto plane = std::make_unique<BandPlane>();
    const size_t outputSize = output.size();
    plane->setData(std::move(output));
    plane->setEndian(BandPlane::Endian::Dependant);
    plane->setCompression(0x11);

    DEBUGMSG(_("Finished band encoding: type=0x11, size=%zu"), outputSize);

    return plane;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */
