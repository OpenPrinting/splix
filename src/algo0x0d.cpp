/*
 *         algo0x0d.cpp   SpliX is Copyright 2006-2008 by Aurélien Croc
 *                        This file is a SpliX derivative work
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
#include "algo0x0d.h"
#include <cstring>
#include <algorithm>
#include <span>
#include "errlog.h"
#include "request.h"
#include "printer.h"
#include "bandplane.h"



// LIFECYCLE: Managed by compiler defaults in the header.



/*
 * Method for two-byte packet encoding.
 */
inline void Algo0x0D::writeTwoBytesPacket( std::vector<uint8_t> &output,
                            int32_t accumulatedHorizontalOffsetValue,
                            int32_t accumulatedVerticalOffsetValue,
                            uint32_t accumulatedRunCount )
{
    /* Encodes the run count and vertical displacement in the first byte. */
    output.push_back( static_cast<uint8_t>((accumulatedVerticalOffsetValue << 6) | accumulatedRunCount) );

    /* Encodes the offset value in the second byte. This is signed. */
    output.push_back( static_cast<uint8_t>(0xFF & accumulatedHorizontalOffsetValue) );
}



/*
 * Method for four-byte packet encoding.
 * Encodes the pen offset value as a 14-bit signed value in the first two bytes.
 */
inline void Algo0x0D::writeFourBytesPacket( std::vector<uint8_t> &output,
                            int32_t accumulatedHorizontalOffsetValue,
                            int32_t accumulatedVerticalOffsetValue,
                            uint32_t accumulatedRunCount )
{
    /* Encodes the upper 6-bit value of the offset value. */
    output.push_back( 0x80 | static_cast<uint8_t>((0x00003F00 & accumulatedHorizontalOffsetValue) >> 8) );

    /* Encode the lower 8-bit value of the offset value. */
    output.push_back( static_cast<uint8_t>(0x000000FF & accumulatedHorizontalOffsetValue) );

    /* Encode the run count as a unsigned 12-bit value in the third byte. */
    output.push_back( 0x80 | static_cast<uint8_t>((accumulatedVerticalOffsetValue << 4) | (accumulatedRunCount >> 8)) );
    
    /* Encode the last byte of run count. */
    output.push_back( static_cast<uint8_t>(0x000000FF & accumulatedRunCount) );
}



/*
 * Method for six-byte packet encoding.
 * Encodes the offset value as a 24-bit unsigned value in the second,
 * third and fourth bytes.
 */
inline void Algo0x0D::writeSixBytesPacket( std::vector<uint8_t> &output,
                            uint32_t accumulatedCombinedOffsetValue,
                            uint32_t accumulatedRunCount )
{
    /* Write the packet header constant in the first byte. */
    output.push_back( 0xC0 );

    /* Encodes the upper 8-bit value of the 24-bit offset value. */
    output.push_back( static_cast<uint8_t>((0x00FF0000 & accumulatedCombinedOffsetValue) >> 16) );

    /* Encodes the middle 8-bit value of the 24-bit offset value. */
    output.push_back( static_cast<uint8_t>((0x0000FF00 & accumulatedCombinedOffsetValue) >> 8) );

    /* Encodes the remaining 8-bit value of the 24-bit offset value. */
    output.push_back( static_cast<uint8_t>(0x000000FF & accumulatedCombinedOffsetValue) );

    /* Encodes the run counts as unsigned 14-bit value in the fifth and sixth byte. */
    /* Encodes the run counts in upper 6-bits. */
    output.push_back( 0xC0 | static_cast<uint8_t>(accumulatedRunCount >> 8) );

    /* Encodes the run count lower 8-bits value. */
    output.push_back( static_cast<uint8_t>(0x000000FF & accumulatedRunCount) );
}



/*
 *  Modifications: Jun-29-2008, Jul-08-2008, Oct-03-2009, Oct-04-2009.
 */
inline void Algo0x0D::selectPacketSize(
                            std::vector<uint8_t> &output,
                            uint32_t preAccumulatedHorizontalOffsetValue,
                            uint32_t accumulatedHorizontalOffsetValue,
                            uint32_t currentHorizontalPenPosition,
                            uint32_t accumulatedRunCount,
                            uint32_t consecutiveBlankScanLines,
                            uint32_t currentVerticalPenPosition,
                            const uint32_t wrapWidth )
{
    /* Set the initial vertical offset value. */
    int32_t verticalOffsetValue = consecutiveBlankScanLines;

    /* Set the initial horizontal offset value. */
    int32_t horizontalOffsetValue = accumulatedHorizontalOffsetValue;

    /* Verify if this is the first formed packet of the scan-line
       and that it is not the first top-most scan-line of the given band.
       Can be verified because on every beginning of a scan-line work, the 
       pre-accumulated horizontal offset value is zero. */
    if ( ( 0 == preAccumulatedHorizontalOffsetValue ) &&
                                ( 0 < currentVerticalPenPosition ) ) {
        /* Evaluate pixel distance between previous and current pen position
           to find the relative horizontal offset value. */
        horizontalOffsetValue -= currentHorizontalPenPosition;

        /* Adjust by +1, when any of the previous scan-lines is not blank. */
        if ( consecutiveBlankScanLines < currentVerticalPenPosition ) {
            verticalOffsetValue++;
        }

    } else {

        /* Process a sequential packet for current scan-line.
           The pre-accumulated offset value must be added, this was the 
           previous packet's run count value. */
        horizontalOffsetValue += preAccumulatedHorizontalOffsetValue;

    }

    /* Choosing the packet size. */
    if ( ( 127 >= horizontalOffsetValue )
                                && ( -128 <= horizontalOffsetValue ) 
                                && ( 63 >= accumulatedRunCount )
                                && ( 1 >= verticalOffsetValue ) ) {

	/* Issue an encoded 2-byte packet. */
        writeTwoBytesPacket( output,
                                horizontalOffsetValue,
                                verticalOffsetValue,
                                accumulatedRunCount );

    } else if ( ( 8191 >= horizontalOffsetValue )
                                && ( -8192 <= horizontalOffsetValue )
                                && ( 4095 >= accumulatedRunCount )
                                && ( 3 >= verticalOffsetValue ) ) {

	/* Issue an encoded 4-byte packet. */
        writeFourBytesPacket( output,
                                horizontalOffsetValue,
                                verticalOffsetValue,
                                accumulatedRunCount );

    } else {

        /* Issue an encoded 6-byte packet. */ 
        writeSixBytesPacket( output, 
			        wrapWidth * verticalOffsetValue
                                + horizontalOffsetValue,
                                accumulatedRunCount );

    }
}



/*
 * Main algorithm 0xd encoder.
 */
SP::Result<std::unique_ptr<BandPlane>> Algo0x0D::compress(const Request& request, std::span<const uint8_t> data,
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

    /* Working width row-bytes. How many bytes need to store a working width
     number of pixels. */
    const uint32_t workWidthRowBytes = ( workWidth + 7 ) / 8;

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

