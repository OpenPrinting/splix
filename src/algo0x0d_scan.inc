SP::Result<std::unique_ptr<BandPlane>> Algo0x0D::compress([[maybe_unused]] const Request& request, std::span<const uint8_t> data,
        uint32_t width, uint32_t height)
{
    /* Basic parameters validation. */
    if (data.empty() || !height || !width ) {
        ERRORMSG(_("Invalid given data for compression: 0xd"));
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    /* Input sanitization: prevent absurd dimensions that could lead to overflow or resource exhaustion. */
    if (width > 65536 || height > 65536) {
        ERRORMSG(_("Absurd dimensions for compression: %u x %u"), width, height);
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    /* We will interpret the band height of 128 pixels as 600 DPI printing
     request. Likewise, height of 64 pixels as 300 DPI printing. */
    if ( ! ( 128 == height || 64 == height ) ) {
        ERRORMSG(_("Invalid band height for compression: 0xd"));
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    /* Set the hardware wrapping width for six-byte type packet format. */
    const uint32_t wrapWidth = ( 64 == height ) ? 0x09A0 : 0x1360;

    /* These are the limits that an encoded scan-line is the allowed to produce
     until encoding is given up. 250 bytes for 300 DPI, 122 bytes for 600 DPI.*/
    const uint32_t maxEncodedBytesPerScanLine = ( 64 == height ) ?
        250 : 122;

    /* Estimate a output buffer size limit equal to 256 bytes times the bitmap
     height for 300 DPI printing, and 128 bytes times the bitmap height for
     600 DPI printing. */
    const uint32_t maximumBufferSize = ( 64 == height ) ?
        256 * height + 4: 128 * height + 4;

    /* Safety cap on buffer size (512MB) to prevent OOM attacks. */
    if (maximumBufferSize > 512 * 1024 * 1024) {
        ERRORMSG(_("Compression buffer size exceeds safety limit: %u"), maximumBufferSize);
        return SP::Unexpected(SP::Error::MemoryError);
    }

    /* Create the output buffer for work. */
    std::vector<uint8_t> output;
    try {
        output.reserve(maximumBufferSize);
    } catch (const std::bad_alloc&) {
        ERRORMSG(_("Failed to allocate compression buffer for 0xd"));
        return SP::Unexpected(SP::Error::MemoryError);
    }


    /* Encoded data size of current scan-line. */
    uint32_t encodedScanLineSize = 0;

    /* Mask to track the bits in raw data-byte that is being processed. */
    uint8_t bitMask = 0x80;

    /* Accumulation of the number of (black) pixel runs. */
    uint32_t accumulatedRunCount = 0;

    /* Accumulation of horizontal offset value in pixels. */
    uint32_t accumulatedHorizontalOffsetValue = 0;

    /* Index for the raw data byte in the current scanline. */
    uint32_t rowByteIndex = 0;

    /* Current absolute horizontal pen position. */
    uint32_t currentHorizontalPenPosition = 0;

    /* Run counts are accounted for as an offset after each packet encodings. */
    uint32_t preAccumulatedHorizontalOffsetValue = 0;

    /* Current absolute vertical pen position. */
    uint32_t currentVerticalPenPosition = 0;

    /* Number of row-bytes in the bitmap. */
    const uint32_t rowBytes = ( width + 7 ) / 8;

    /* This is the working and therefore the effective printing width.
    Crop the working width if bitmap is longer than wrap width. */
    const uint32_t workWidth = ( width > wrapWidth ) ? wrapWidth : width;

    /* Number of pixels left to be processed in the scanline. */
    uint32_t pixelsLeftInScanline = workWidth;

    /* Number of accumulated consecutive blank scanline. */
    uint32_t consecutiveBlankScanLines = 0;

    /* Current scanline view */
    std::span<const uint8_t> scanline = data;

    /* Main encoding loop. */
    while (currentVerticalPenPosition < height) {

        // Lambda to scan consecutive bits of a specific value (set/unset)
        auto scanConsecutiveBits = [&](bool targetValue) -> uint32_t {
            uint32_t count = 0;
            while (pixelsLeftInScanline > 0) {
                if (rowByteIndex >= scanline.size()) {
                    pixelsLeftInScanline = 0;
                    break;
                }

                const bool isSet = (scanline[rowByteIndex] & bitMask) != 0;
                if (isSet != targetValue) {
                    break;
                }

                // Bit matches target value
                count++;
                pixelsLeftInScanline--;

                // Advance bit mask and byte index
                bitMask >>= 1;
                if (bitMask == 0) {
                    bitMask = 0x80;
                    rowByteIndex++;
                }
            }
            return count;
        };

        /* 1. Scan for the offset (consecutive white/unset pixels). */
