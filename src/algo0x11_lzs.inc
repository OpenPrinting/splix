SP::Result<> Algo0x11::_compress(std::span<const uint8_t> data, std::vector<uint8_t> &output)
{
    uint32_t r=0, uncompSize=0;
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
