/*
 *      algo0x0d.cpp              (C) 2008, Leonardo H. Souza Hamada
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
 *  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 * 
 */

/*
 * This file is a submission to the Splix project
 * by Leonardo H. Souza Hamada <leonardohamada AT hotmail DOT com>.
 * You can adapt, correct, optimize and organize according to your needs.
 * 14/05/2008, Belém-PA, Pará, Brazil.
 *
 * Code format updated by Aurélien Croc
 */
#include "algo0x0d.h"
#include <string.h>
#include "errlog.h"
#include "request.h"
#include "printer.h"
#include "bandplane.h"


/*
 * Constructeur - Destructeur
 * Constructor
 */
Algo0x0D::Algo0x0D()
{
}

Algo0x0D::~Algo0x0D()
{
}



/* 
 * Méthode d'encodage des paquets de deux octets
 * Method for two-byte packet encoding.
 * Contribution by Leonardo Hamada.
 */
inline void Algo0x0D::writeTwoBytesPacket(unsigned char* output, 
    unsigned long &outputSize, int32_t accumulatedHorizontalOffsetValue, 
    int32_t displacementValue, uint32_t accumulatedRunCount)
{
	/* Encode the run count and vertical displacement in the first byte. */
	output[outputSize++] = (unsigned char) ((displacementValue << 6) | 
        accumulatedRunCount);
	/* Encode the offset value in the second byte. This is signed. */
	output[outputSize++] = (unsigned char) (0xFF & 
        accumulatedHorizontalOffsetValue );
}



/*
 * Méthode d'encodage des paquets de quatre octets
 * Method for four-byte packet encoding.
 * Contribution by Leonardo Hamada.
 */
inline void Algo0x0D::writeFourBytesPacket(unsigned char* output, 
    unsigned long &outputSize, int32_t accumulatedHorizontalOffsetValue, 
    int32_t displacementValue, uint32_t accumulatedRunCount)
{
	/* 
     * Encode the offset value as a 14-bit signed value in the first two bytes.
     */

	/* Encode the upper 6-bit value of the offset value. */	
	output[outputSize++]  = 0x80 | (unsigned char) ((0x00003F00 & 
        accumulatedHorizontalOffsetValue) >> 8);
	/* Encode the lower 8-bit value of the offset value. */
	output[outputSize++] = (unsigned char) (0x000000FF & 
        accumulatedHorizontalOffsetValue);
	/* Encode the run count as a unsigned 12-bit value in the third byte. */
	output[outputSize++] = 0x80 | (unsigned char) ((displacementValue << 4) | 
        (accumulatedRunCount >> 8));	
	output[outputSize++] = (unsigned char) (0x000000FF & accumulatedRunCount);
}



/*
 * Méthode d'encodage des paquets de six ocetets
 * Method for six-byte packet encoding.
 * Contribution by Leonardo Hamada.
 */
inline void Algo0x0D::writeSixBytesPacket(unsigned char* output, 
    unsigned long &outputSize, int32_t displacementValue, 
    uint32_t accumulatedRunCount)
{
	/* Write the packet header constant in the first byte. */
	output[outputSize++] = 0xC0;

	/* 
     * Encode the offset value as a 24-bit unsigned value in the second, 
     * third and fourth bytes.
     */

    /* Encode the upper 8-bit value of the 24-bit offset value. */	
	output[outputSize++] = (unsigned char) (displacementValue >> 16);
	/* Encode the middle 8-bit value of the 24-bit offset value. */
	output[outputSize++] = (unsigned char) ((0x0000FF00 & displacementValue) >>
        8);
	/* Encode the remaining 8-bit value of the 24-bit offset value. */
	output[outputSize++] = (unsigned char) ( 0x000000FF & displacementValue);	
	/* 
     * Encode the run count as a unsigned 14-bit value in the fifth and sixth 
     * byte
     */
	/* Encode the run count upper 6-bits. */
	output[outputSize++] = 0xC0 | (unsigned char) (accumulatedRunCount >> 8);
	/* Encode the run count lower 8-bits value. */
	output[outputSize++] = (unsigned char) (0x000000FF & accumulatedRunCount);
}



/*
 * Encodage (retourne la taille des paquets enregistrés)
 * The encoder.
 * Returns the size of packet just recorded.
 * Contribution by Leonardo Hamada.
 */
inline void Algo0x0D::encodeInPacket(unsigned char* output, 
    unsigned long &outputSize, uint32_t preAccumulatedHorizontalOffsetValue, 
    uint32_t accumulatedHorizontalOffsetValue, 
    uint32_t currentHorizontalPenPosition, uint32_t pixelsLeftInScanline, 
    uint32_t accumulatedRunCount, uint32_t consecutiveBlankScanLines, 
    uint32_t currentVerticalPenPosition, unsigned long width)
{
	/* 
     * Verify that this is the first offset value on the scanline, 
     * but not the first scanline.
     */
	/* 
     * On every new scanline start, the pre-accumulated offset value must be 
     * zero. Otherwise is non-zero.
     */
    if ((0 == preAccumulatedHorizontalOffsetValue) && (0 < 
            currentVerticalPenPosition)) {
        /* 
         * Value to evaluate distante between previous and current pen 
         * position to output correct packet format.
         */
        int32_t distanceBetweenCurrentAndTheNewPenPosition = 
            currentHorizontalPenPosition - accumulatedHorizontalOffsetValue;
        /* 
         * Calculated necessary pen displacement in pixels to position it 
         * in correct place.
         */
        int32_t displacementValue = 0;


        if (distanceBetweenCurrentAndTheNewPenPosition <= 0) {
            /* Turn the displacement value into a positive integer value. */
            distanceBetweenCurrentAndTheNewPenPosition = 
                -distanceBetweenCurrentAndTheNewPenPosition;

            if ((distanceBetweenCurrentAndTheNewPenPosition <= 127) &&
                    (accumulatedRunCount <= 63) && (0 == 
                    consecutiveBlankScanLines)) {
                displacementValue = 1;
                writeTwoBytesPacket(output, outputSize,
                    distanceBetweenCurrentAndTheNewPenPosition,
                    displacementValue, accumulatedRunCount);
            } else if ((distanceBetweenCurrentAndTheNewPenPosition <= 8191) &&
                    (accumulatedRunCount <= 4095) && (2 >= 
                    consecutiveBlankScanLines)) {
                displacementValue = 1 + consecutiveBlankScanLines; 
                writeFourBytesPacket(output, outputSize,
                    distanceBetweenCurrentAndTheNewPenPosition,
                    displacementValue, accumulatedRunCount);
            } else {
                displacementValue = width * (1 + consecutiveBlankScanLines) +
                    distanceBetweenCurrentAndTheNewPenPosition;
                writeSixBytesPacket(output, outputSize,
                    displacementValue, accumulatedRunCount);
            }
        } else {
            if ((distanceBetweenCurrentAndTheNewPenPosition <= 128) &&
                    (accumulatedRunCount <= 63) && (0 == 
                    consecutiveBlankScanLines)) {
                displacementValue = 1;
                writeTwoBytesPacket(output, outputSize,
                    -distanceBetweenCurrentAndTheNewPenPosition,
                    displacementValue, accumulatedRunCount);
            } else if ((distanceBetweenCurrentAndTheNewPenPosition <= 8192 ) &&
                    (accumulatedRunCount <= 4095) && (2 >= 
                    consecutiveBlankScanLines)) {
                displacementValue = 1 + consecutiveBlankScanLines; 
                writeFourBytesPacket(output, outputSize,
                    -distanceBetweenCurrentAndTheNewPenPosition,
                    displacementValue, accumulatedRunCount);
            } else {
                displacementValue = width * (1 + consecutiveBlankScanLines) -
                    distanceBetweenCurrentAndTheNewPenPosition;
                writeSixBytesPacket(output, outputSize,
                    displacementValue, accumulatedRunCount);
            }
        }
    } else {
        /* Process the rest of packets in the scanline. */
        /* Before that, add the pre-accumulated offset value. */
        uint32_t recalculatedOffset = accumulatedHorizontalOffsetValue + 
            preAccumulatedHorizontalOffsetValue;

        if ((recalculatedOffset <= 127) && (accumulatedRunCount <= 63)) {
            writeTwoBytesPacket(output, outputSize, recalculatedOffset,
                0, accumulatedRunCount);
        } else if ((recalculatedOffset <= 8191) && (accumulatedRunCount <= 
                4095)) {
            writeFourBytesPacket(output, outputSize, recalculatedOffset,
                0, accumulatedRunCount);
        } else {
            writeSixBytesPacket(output, outputSize, recalculatedOffset,
                accumulatedRunCount);
        }
    }
}



/*
 * Méthode de compression
 * Compression routine.
 * Contribution by Leonardo Hamada.
 */
BandPlane* Algo0x0D::compress(const Request& request, unsigned char *data, 
    unsigned long width, unsigned long height)
{
	/* 
     * This scheme can sometimes be so inefficient that it may be possible to 
     * construct an artificial scenario of encoding just 1 byte per pixel, 
     * for example, in a 1-pixel sized checkerboard pattern.
     * So we build a buffer with a sufficient size to deal with such case. 
     * Unfortunately, for now. 
     */
	unsigned long outputSize = 0, maximumBufferSize = (width + 4) * height;
	/* Create the output buffer. */
	unsigned char *output;
    unsigned char *resizedOutput;
	BandPlane *plane;
	/* Pointer to 8-bit chunks of data. */
	uint8_t *rawDataBytePointer = data;
	/* Mask to track the bits in raw data-byte that is being processed. */
	uint8_t _8bitMask = 0x80;
	/* Accumulation of the number of (black) pixel runs. */
	uint32_t accumulatedRunCount = 0; 
	/* Accumulation of horizontal offset value in pixels. */
	uint32_t accumulatedHorizontalOffsetValue = 0;
	/* Index for the raw data byte in the current scanline. */
	uint32_t accumulatedRowByteIndex = 0;
	/* Current absolute horizontal pen position. */
	uint32_t currentHorizontalPenPosition = 0;
	/* 
     * The run count must be accounted for as an offset after each packet 
     * encoding.
     */ 
	uint32_t preAccumulatedHorizontalOffsetValue = 0;
	/* Current absolute vertical pen position. */
	uint32_t currentVerticalPenPosition = 0;
	/* Number of row-bytes in the bitmap. */
	uint32_t rowBytes = (7 + width) / 8; 
	/* Number of pixels left to be processed in the scanline. */
	uint32_t pixelsLeftInScanline = width;
	/* Number of acumulated consecutive blank scanline. */
	uint32_t consecutiveBlankScanLines = 0;

    if (!data || !maximumBufferSize) {
        ERRORMSG(_("Invalid given data for compression (0x0D)"));
        return NULL;
    }
    output = new unsigned char[maximumBufferSize];

    /* Main encoding loop. */
    while (currentVerticalPenPosition < height) {
        /* Scan for offset value. */
        while (accumulatedRowByteIndex < rowBytes) {

            /* 
             * Check current data-byte against the mask for a zero-bit (unset) 
             * value. 
             */ 
            if ((0 == (_8bitMask & rawDataBytePointer[accumulatedRowByteIndex]))
                    && (pixelsLeftInScanline > 0)) {
                /* Account for a blank pixel. */
                accumulatedHorizontalOffsetValue++;
                /* Make a discount for previous processed pixel. */
                pixelsLeftInScanline--;
            } else {
                /* Exit current loop for scanning of offset values. */
                break;
            }

            /* Rotating the bit mask. */
            _8bitMask = _8bitMask >> 1;
            /* Reset the mask if needed. */
            if (0 == _8bitMask) {
                _8bitMask = 0x80;
                /* Advance the row byte index. */
                accumulatedRowByteIndex++;
            }
        }

        /* Now, scan for run count. */
        while (accumulatedRowByteIndex < rowBytes)
        {
            /* 
             * Check current data-byte against the mask for an one-bit (set) 
             * value.
             */ 
            if ((0 != (_8bitMask & rawDataBytePointer[accumulatedRowByteIndex]))
                    && (pixelsLeftInScanline > 0)) {
                /* Account for a black pixel. */
                accumulatedRunCount++;
                /* Make a discount for previous processed pixel. */
                pixelsLeftInScanline--;
            } else {
                /* Exit current loop for scanning of run count. */
                break;
            }

            /* Rotating the bit mask. */
            _8bitMask = _8bitMask >> 1;
            /* Reset the mask if needed. */
            if (0 == _8bitMask) {
                _8bitMask = 0x80;
                /* Advance the row byte index. */
                accumulatedRowByteIndex++;
            }
        }

        /* 
         * If we are here is because we have an offset value and a run 
         * count pair.
         */
        /* Verify if it's a blank scanline before proceeding. */
        if ((accumulatedHorizontalOffsetValue == width) && (0 == 
                accumulatedRunCount)) {
            /* We encountered a blank scanline, so account for it. */
            consecutiveBlankScanLines++;
        } 
        
        if (0 <  accumulatedRunCount) {
            /* We have pixels to encode, proceed. */
            /* Encode it now. */
            if (8 < maximumBufferSize - outputSize) {
                /* Acumulate the output packet size. */
                encodeInPacket(output, outputSize,
                    preAccumulatedHorizontalOffsetValue,
                    accumulatedHorizontalOffsetValue,
                    currentHorizontalPenPosition, pixelsLeftInScanline, 
                    accumulatedRunCount, consecutiveBlankScanLines, 
                    currentVerticalPenPosition, width);
            } else {
                /* We failed! */
                ERRORMSG(_("Out of buffer space."));
                delete[] output;
                return NULL;
            }

            /* After encoding, reset the blank scanline counter. */
            consecutiveBlankScanLines = 0;
            /* Update the pen position. */
            currentHorizontalPenPosition = width - pixelsLeftInScanline - 
                accumulatedRunCount;
            /* Must pre-accumulate the offset value. */
            preAccumulatedHorizontalOffsetValue = accumulatedRunCount;			
        }

        if (0 == pixelsLeftInScanline) {
            /* No more pixels left in this scanline, so go to next one. */
            rawDataBytePointer = &rawDataBytePointer[rowBytes];
            /* Advance the vertical pen position. */
            currentVerticalPenPosition++;
            /* Reset control variables to initial values. */
            accumulatedRowByteIndex = 0;
            _8bitMask = 0x80;
            pixelsLeftInScanline = width;
            /* 
             * Reset the pre-accumulated offset value at each scanline 
             * beginning.
             */
            preAccumulatedHorizontalOffsetValue = 0;
        }

        /* Always reset the run count and the offset value . */
        accumulatedRunCount = 0;
        accumulatedHorizontalOffsetValue = 0;
    }

    /* Append the 0x0000 16-bit value. */
    output[outputSize++] = 0x00;
    output[outputSize++] = 0x00;
    /* 
     * Because the initial buffer is so large, perhaps it is a good idea
     * to make the output buffer to just fit the data before ending this band 
     * encoding.
     */
    resizedOutput = new unsigned char[outputSize];
    memcpy(resizedOutput, output, outputSize);
    delete[] output;
    /* Register the result into a band plane. */
    plane = new BandPlane();
    plane->setData(resizedOutput, outputSize);
    plane->setEndian(BandPlane::BigEndian);

    return plane;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

