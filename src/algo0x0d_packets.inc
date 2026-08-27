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
