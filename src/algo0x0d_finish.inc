        accumulatedHorizontalOffsetValue = scanConsecutiveBits(false);

        /* 2. Scan for the run (consecutive black/set pixels). */
        accumulatedRunCount = scanConsecutiveBits(true);

        /* 3. Handle the captured run/offset pair. */
        if (accumulatedHorizontalOffsetValue == workWidth && accumulatedRunCount == 0) {
            /* This was an entirely blank scanline. */
            consecutiveBlankScanLines++;
        } else if (accumulatedRunCount > 0) {
            /* We have actual pixels to encode. */
            const size_t outputSizeBefore = output.size();

            // Guard against buffer overflow (though reserved, we stay safe)
            if (output.size() + 12 > maximumBufferSize) {
                ERRORMSG(_("Compression buffer limit reached: 0xd"));
                return SP::Unexpected(SP::Error::MemoryError);
            }

            selectPacketSize(output,
                            preAccumulatedHorizontalOffsetValue,
                            accumulatedHorizontalOffsetValue,
                            currentHorizontalPenPosition,
                            accumulatedRunCount,
                            consecutiveBlankScanLines,
                            currentVerticalPenPosition,
                            wrapWidth);

            const uint32_t bytesProduced = static_cast<uint32_t>(output.size() - outputSizeBefore);
            encodedScanLineSize += bytesProduced;

            /* Check if this scanline's complexity exceeds the algorithm's density limit. */
            if (encodedScanLineSize > maxEncodedBytesPerScanLine) {
                DEBUGMSG(_("Scanline too complex for 0xd (size=%u), giving up."), encodedScanLineSize);
                return SP::Unexpected(SP::Error::LogicError);
            }

            /* Reset the blank scanline counter since we've now hit a non-blank line. */
            consecutiveBlankScanLines = 0;

            /* Update the pen's horizontal tracking. */
            currentHorizontalPenPosition = workWidth - pixelsLeftInScanline - accumulatedRunCount;

            /* Update pre-accumulation for the next packet on this line. */
            preAccumulatedHorizontalOffsetValue = accumulatedRunCount;
        }

        /* 4. Check if the scanline is finished. */
        if (pixelsLeftInScanline == 0) {
            /* Advance to the next scanline. */
            if (++currentVerticalPenPosition < height) {
                if (scanline.size() >= rowBytes) {
                    scanline = scanline.subspan(rowBytes);
                } else {
                    scanline = {}; // Safety
                }
            }

            /* Reset scanline-specific counters. */
            encodedScanLineSize = 0;
            rowByteIndex = 0;
            bitMask = 0x80;
            pixelsLeftInScanline = workWidth;
            preAccumulatedHorizontalOffsetValue = 0;
        }

        /* Ensure we don't carry over run/offset counts to the next iteration. */
        accumulatedRunCount = 0;
        accumulatedHorizontalOffsetValue = 0;
    }

    /* Zero value byte padding for data size alignment to 4-byte boundary. */
    uint32_t zerosPad = 4 - (static_cast<uint32_t>(output.size()) % 4);

    /* Pad anyway even if already aligned. */
    if (zerosPad > 0 && zerosPad < 4) {
        if (output.size() + zerosPad <= maximumBufferSize) {
            output.insert(output.end(), zerosPad, 0);
        } else {
            ERRORMSG(_("No buffer during padding: 0xd"));
            return SP::Unexpected(SP::Error::MemoryError);
        }
    }


    /* Prepare to return data encoded by algorithm 0xd. */
    auto plane = std::make_unique<BandPlane>();

    plane->setData( std::move(output) );
    plane->setEndian( BandPlane::Endian::Dependant );
    plane->setCompression( 0xd );

    /* Finished this band encoding. */
    DEBUGMSG(_("Finished band encoding: type=0xd, size=%zu"), plane->dataSize());

    return plane;
}
