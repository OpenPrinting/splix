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
#include <new>
#include <string.h>
#include "errlog.h"
#include "request.h"
#include "printer.h"
#include "bandplane.h"



/*
 * Constructor.
 */
Algo0x0D::Algo0x0D()
{
}

Algo0x0D::~Algo0x0D()
{
}



/*
 * Method for two-byte packet encoding.
 */
inline void Algo0x0D::writeTwoBytesPacket( unsigned char * output,
                            unsigned long & outputSize,
                            long int accumulatedHorizontalOffsetValue,
                            long int accumulatedVerticalOffsetValue,
                            unsigned long accumulatedRunCount )
{
    /* Encodes the run count and vertical displacement in the first byte. */
    output[ outputSize++ ] =
        ( unsigned char )( ( accumulatedVerticalOffsetValue << 6 ) |
	                     accumulatedRunCount );

    /* Encodes the offset value in the second byte. This is signed. */
    output[ outputSize++ ] =
        ( unsigned char )( 0xFF & accumulatedHorizontalOffsetValue );
}



/*
 * Method for four-byte packet encoding.
 * Encodes the pen offset value as a 14-bit signed value in the first two bytes.
 */
inline void Algo0x0D::writeFourBytesPacket( unsigned char * output,
                            unsigned long & outputSize,
                            long int accumulatedHorizontalOffsetValue,
                            long int accumulatedVerticalOffsetValue,
                            unsigned long accumulatedRunCount )
{
    /* Encodes the upper 6-bit value of the offset value. */
    output[ outputSize++ ] = 0x80 |
    ( unsigned char )( ( 0x00003F00 & accumulatedHorizontalOffsetValue ) >> 8 );

    /* Encode the lower 8-bit value of the offset value. */
    output[ outputSize++ ] =
            ( unsigned char )( 0x000000FF & accumulatedHorizontalOffsetValue );

    /* Encode the run count as a unsigned 12-bit value in the third byte. */
    output[ outputSize++ ] = 0x80 |
            ( unsigned char )( ( accumulatedVerticalOffsetValue << 4 ) |
                               ( accumulatedRunCount >> 8 ) );
    /* Encode the last byte of run count. */
    output[ outputSize++ ] =
            ( unsigned char )( 0x000000FF & accumulatedRunCount );
}



/*
 * Method for six-byte packet encoding.
 * Encodes the offset value as a 24-bit unsigned value in the second,
 * third and fourth bytes.
 */
inline void Algo0x0D::writeSixBytesPacket( unsigned char * output,
                            unsigned long & outputSize,
                            long int accumulatedCombinedOffsetValue,
                            unsigned long accumulatedRunCount )
{
    /* Write the packet header constant in the first byte. */
    output[ outputSize++ ] = 0xC0;

    /* Encodes the upper 8-bit value of the 24-bit offset value. */
    output[ outputSize++ ] =
                ( unsigned char )( ( 0x00FF0000 & 
                                     accumulatedCombinedOffsetValue ) >> 16 );

    /* Encodes the middle 8-bit value of the 24-bit offset value. */
    output[ outputSize++ ] =
                ( unsigned char )( ( 0x0000FF00 &
                                     accumulatedCombinedOffsetValue ) >> 8 );

    /* Encodes the remaining 8-bit value of the 24-bit offset value. */
    output[ outputSize++ ] =
                ( unsigned char )( 0x000000FF &
                                   accumulatedCombinedOffsetValue );

    /* Encodes the run counts as unsigned 14-bit value in the fifth and
       sixth byte. */

    /* Encodes the run counts in upper 6-bits. */
    output[ outputSize++ ] = 0xC0 |
                ( unsigned char )( accumulatedRunCount >> 8 );

    /* Encodes the run count lower 8-bits value. */
    output[ outputSize++ ] =
                ( unsigned char )( 0x000000FF & accumulatedRunCount );
}



/*
 *  Modifications: Jun-29-2008, Jul-08-2008, Oct-03-2009, Oct-04-2009.
 */
inline void Algo0x0D::selectPacketSize(
                            unsigned char * output,
                            unsigned long & outputSize,
                            unsigned long preAccumulatedHorizontalOffsetValue,
                            unsigned long accumulatedHorizontalOffsetValue,
                            unsigned long currentHorizontalPenPosition,
                            unsigned long accumulatedRunCount,
                            unsigned long consecutiveBlankScanLines,
                            unsigned long currentVerticalPenPosition,
                            const unsigned long wrapWidth )
{
    /* Set the initial vertical offset value. */
    long int verticalOffsetValue = consecutiveBlankScanLines;

    /* Set the initial horizontal offset value. */
    long int horizontalOffsetValue = accumulatedHorizontalOffsetValue;

    /* Verify if this is the first formed packet of the scan-line
       and that it is not the first top-most scan-line of the given band.
       Can be verified because on every beginning of a scan-line work, the 
       pre-accumulated horizontal offset value is zero. */
    if ( ( 0 == preAccumulatedHorizontalOffsetValue ) &&
                                ( 0 < currentVerticalPenPosition ) ) {
        /* Evaluate pixel distance between previous and current pen position
           to find the relative horizontal offset value. */
        horizontalOffsetValue -= currentHorizontalPenPosition;

        /* Adjust by +1, when any of the previous scan-lines is not blank.
           Must account for three cases:
             A - When the band begins with any number of blank scan-lines.
             B - When the current non-blank scan-line follow a blank scan-line,
                 but there are non-blank scan-lines previously.
	     C - When the previous scan-line was not blank.
           Notice that when any of the previous scan-lines is non-blank,
           the blank scan-line counter is always less than and not equal to
           the value of the vertical pen position. */
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
        writeTwoBytesPacket( output, outputSize,
                                horizontalOffsetValue,
                                verticalOffsetValue,
                                accumulatedRunCount );

    } else if ( ( 8191 >= horizontalOffsetValue )
                                && ( -8192 <= horizontalOffsetValue )
                                && ( 4095 >= accumulatedRunCount )
                                && ( 3 >= verticalOffsetValue ) ) {

	/* Issue an encoded 4-byte packet. */
        writeFourBytesPacket( output, outputSize,
                                horizontalOffsetValue,
                                verticalOffsetValue,
                                accumulatedRunCount );

    } else {

        /* Issue an encoded 6-byte packet. */ 
        writeSixBytesPacket( output, outputSize, 
			        wrapWidth * verticalOffsetValue
                                + horizontalOffsetValue,
                                accumulatedRunCount );

    }

    /* Finished one packet encoding. */
}



/*
 * Main algorithm 0xd encoder.
 */
BandPlane * Algo0x0D::compress(const Request & request, unsigned char *data,
        unsigned long width, unsigned long height)
{
    /* Basic parameters validation. */
    if ( !data || !height || !width ) {
        ERRORMSG(_("Invalid given data for compression: 0xd"));
        return NULL;
    }

    /* We will interpret the band heigth of 128 pixels as 600 DPI printing
     request. Likewise, height of 64 pixels as 300 DPI printing. */
    if ( ! ( 128 == height || 64 == height ) ) {
        ERRORMSG(_("Invalid band height for compression: 0xd"));
        return NULL;
    }

    /* Set the hardware wrapping width for six-byte type packet format. */
    const unsigned long wrapWidth = ( 64 == height ) ? 0x09A0 : 0x1360;

    /* These are the limits that an encoded scan-line is the allowed to produce
     until encoding is given up. 250 bytes for 300 DPI, 122 bytes for 600 DPI.*/
    const unsigned long maxEncodedBytesPerScanLine = ( 64 == height ) ?
        250 : 122;

    /* Estimate a output buffer size limit equal to 256 bytes times the bitmap
     height for 300 DPI printing, and 128 bytes times the bitmap height for
     600 DPI printing. */
    const unsigned long maximumBufferSize = ( 64 == height ) ?
        256 * height + 4: 128 * height + 4;

    /* Keep track of the size of encoded data. */
    unsigned long outputSize = 0;

    /* Encoded data size of current scan-line. */
    unsigned long encodedScanLineSize = 0;

    /* Create the output buffer for work. */
    unsigned char * output = NULL;

    try {
        output = new unsigned char[ maximumBufferSize ];
    } catch( std::bad_alloc & ) {
        ERRORMSG(_("Could not allocate work buffer for compression: 0xd"));
        return NULL;
    }

    /* Mask to track the bits in raw data-byte that is being processed. */
    unsigned char bitMask = 0x80;

    /* Accumulation of the number of (black) pixel runs. */
    unsigned long accumulatedRunCount = 0;

    /* Accumulation of horizontal offset value in pixels. */
    unsigned long accumulatedHorizontalOffsetValue = 0;

    /* Index for the raw data byte in the current scanline. */
    unsigned long rowByteIndex = 0;

    /* Current absolute horizontal pen position. */
    unsigned long currentHorizontalPenPosition = 0;

    /* Run counts are accounted for as an offset after each packet encodings. */
    unsigned long preAccumulatedHorizontalOffsetValue = 0;

    /* Current absolute vertical pen position. */
    unsigned long currentVerticalPenPosition = 0;

    /* Number of row-bytes in the bitmap. */
    const unsigned long rowBytes = ( width + 7 ) / 8;

    /* This is the working and therefore the effective printing width.
    Crop the working width if bitmap is longer than wrap width. */
    const unsigned long workWidth = ( width > wrapWidth ) ? wrapWidth : width;

    /* Working width row-bytes. How many bytes need to store a working width
     number of pixels. */
    const unsigned long workWidthRowBytes = ( workWidth + 7 ) / 8;

    /* Number of pixels left to be processed in the scanline. */
    unsigned long pixelsLeftInScanline = workWidth;

    /* Number of acumulated consecutive blank scanline. */
    unsigned long consecutiveBlankScanLines = 0;

    /* Main encoding loop. */
    while ( currentVerticalPenPosition < height ) {

        /* Scan for offset value. */
        while ( rowByteIndex < workWidthRowBytes ) {

            /* Check current byte data against the mask for a unset bit. */
            if ( ( 0 == ( bitMask & data[ rowByteIndex ] ) ) &&
                                                ( pixelsLeftInScanline > 0 ) ) {
                /* Account for a blank pixel. */
                accumulatedHorizontalOffsetValue++;

                /* Make a discount for previous processed pixel. */
                pixelsLeftInScanline--;
            } else {
                /* Exit current loop for scanning of offset values. */
                break;
            }

            /* Rotating the bit mask. */
            bitMask >>= 1;

            /* Reset the mask if needed. */
            if ( 0 == bitMask ) {
                bitMask = 0x80;

                /* Advance the row byte index. */
                rowByteIndex++;
            }
        }

        /* Now, scan for run count. */
        while ( rowByteIndex < workWidthRowBytes ) {

            /* Check byte against the mask for an one-bit (set) value. */
            if ( ( 0 != ( bitMask & data[ rowByteIndex ] ) ) &&
                                            ( pixelsLeftInScanline > 0 ) ) {
                /* Account for a black pixel. */
                accumulatedRunCount++;

                /* Make a discount for previous processed pixel. */
                pixelsLeftInScanline--;
            } else {
                /* Exit current loop for scanning of run count. */
                break;
            }

            /* Rotating the bit mask. */
            bitMask >>= 1;

            /* Reset the mask if needed. */
            if ( 0 == bitMask ) {
                bitMask = 0x80;

                /* Advance the row byte index. */
                rowByteIndex++;
            }
        }

        /* We have an offset value and a run count pair. */
        /* Verify if it's a blank scanline before proceeding. */
        if ( ( accumulatedHorizontalOffsetValue == workWidth ) &&
                                            ( 0 == accumulatedRunCount ) ) {
            /* We encountered a blank scanline, so account for it. */
            consecutiveBlankScanLines++;
        } else if ( 0 < accumulatedRunCount ) {

            /* We have pixels to encode, proceed. */
            unsigned long previousOutputSize = outputSize;

           if ( outputSize + 6 + 4  <= maximumBufferSize ) {
                selectPacketSize( output, outputSize,
                        preAccumulatedHorizontalOffsetValue,
                        accumulatedHorizontalOffsetValue,
                        currentHorizontalPenPosition,
                        accumulatedRunCount, consecutiveBlankScanLines,
                        currentVerticalPenPosition, wrapWidth );
            } else {
                /* Here we failed. Unlikely. */
                ERRORMSG(_("Out of buffer space: 0xd"));
                delete [] output;
                return NULL;
            }

            encodedScanLineSize += ( outputSize - previousOutputSize );

            if ( maxEncodedBytesPerScanLine < encodedScanLineSize ) {
                /* We did not fail, but gave up because data is unsuited for
                 encoding by this algorithm. */
                delete [] output;
                return NULL;
            }

            /* After encoding, reset the blank scan-line counter. */
            consecutiveBlankScanLines = 0;

            /* Update the pen position. This is one way of doing it. */
            currentHorizontalPenPosition =
                        workWidth - pixelsLeftInScanline - accumulatedRunCount;

            /* Must pre-accumulate the offset value. */
            preAccumulatedHorizontalOffsetValue = accumulatedRunCount;
        }

        if ( 0 == pixelsLeftInScanline ) {
            /* Advance the vertical pen position. */
            if ( ++currentVerticalPenPosition < height ) {
		/* No more pixels left in this scan-line, so go to the next one. */
		data = & data[ rowBytes ];
	    }

            /* Re-initialize the encoded scan-line data size tracker to zero. */
            encodedScanLineSize = 0;

            /* Reset control variables to initial values. */
            rowByteIndex = 0;

            /* Reset the bit mask. */
            bitMask = 0x80;

            /* Re-init the working width. */
            pixelsLeftInScanline = workWidth;

            /* Reset the pre-accumulated offset value at each scan-line done. */
            preAccumulatedHorizontalOffsetValue = 0;
        }

        /* Always reset the run count and the offset value in every pass. */
        accumulatedRunCount = 0;
        accumulatedHorizontalOffsetValue = 0;
    }

    /* Zero value byte padding for data size alignment to 4-byte boundary. */
    unsigned long zerosPad = 4 - ( outputSize % 4 );

    /* Pad anyway even if already aligned. */
    if ( outputSize + zerosPad <= maximumBufferSize ) {
        while ( zerosPad-- ) {
            output[ outputSize++ ] = 0;
        }
    } else {
        /* Here we failed. Unlikely. */
        ERRORMSG(_("No buffer during padding: 0xd"));
        delete [] output;
        return NULL;
    }

    /* Prepare to return data encoded by algorithm 0xd. */
    BandPlane * plane = new BandPlane();
    
    plane->setData( output, outputSize );
    plane->setEndian( BandPlane::Dependant );
    plane->setCompression( 0xd );

    /* Finished this band encoding. */
    DEBUGMSG(_("Finished band encoding: type=0xd, size=%lu"), outputSize);

    return plane;
}
