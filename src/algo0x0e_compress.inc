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
