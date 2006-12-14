/*
 * 	compress.cpp	        	(C) 2006, Aurélien Croc (AP²C)
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
 *  $Id$
 * 
 */
#include "compress.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static int32_t ptrArray[0x40];
static uint32_t maxSizeArray[0x40];

#define COMPRESS_SAMPLE_RATE   0x800


static int _compare(const void *n1, const void *n2)
{
    // n2 and n1 has been exchanged since the first
    // element of the array MUST be the biggest
    return *(uint32_t *)n2 - *(uint32_t *)n1;
}

int calcOccurs(unsigned char *band, unsigned long bandHeight, 
        unsigned long bandWidth, unsigned long number)
{
    uint32_t occurs[COMPRESS_SAMPLE_RATE * 2];
    size_t i, j, size;

    size = bandWidth * bandHeight;

    // Initialize buffers
    for (i=0; i < COMPRESS_SAMPLE_RATE; i++) {
        occurs[i*2] = 0;
        occurs[i*2 + 1] = i;
    }

    // Calculate the byte occurrence
    for (i=COMPRESS_SAMPLE_RATE; i < size; i += COMPRESS_SAMPLE_RATE) {
        char b = band[i];

        for (j=1; j < COMPRESS_SAMPLE_RATE; j++)
            if (band[i - j] == b)
                occurs[(j-1)*2]++;
    }

    // Order the array
    qsort(occurs, COMPRESS_SAMPLE_RATE, sizeof(uint32_t)*2, _compare);

    // Get the first 0x40 elements
    for (i=0; i < 0x40; i++)
        ptrArray[i] = ~occurs[i*2 + 1] - 1;
    
    // Get the maximum length of a compressed data
    if (number > 0x63  || !number) {
        for (i=0; i < 0x40; i++)
            maxSizeArray[i] = 0x202;
    } else {
        uint32_t l;

        l = 0x6464 / (number << 6);
        for (i=0; i < 0x40; i++) {
            uint32_t v = 0x202 - l * i;

            if (v < 3)
                v = 3;
            maxSizeArray[i] = v;
        }
    }

	return 0;
}

int compressBand(struct BandArray *bandArray, unsigned char *beginIn,
        unsigned long bandWidth, unsigned long bandHeight)
{
    unsigned char *out, *endOut, *in, *endIn, *rawDataPtr = 0;
    size_t max, repCnt, maxRepCnt, rawDataNr = 0;
    int32_t lastPtr = 0, si;
    size_t i, maxPtr;

    // Initialize some variables
    out = bandArray->next;
    endOut = bandArray->next + bandWidth * bandHeight;
    in = beginIn;
    endIn = beginIn + bandWidth * bandHeight;

    // Print the table
    for (i=0; i < 0x40; i++) {
        *(int16_t *)out = ~(int16_t)ptrArray[i];
        out += 2;
        if (ptrArray[i] < lastPtr)
            lastPtr = ptrArray[i];
    }

    // Print the first uncompressed bytes
    lastPtr = ~lastPtr;
    if (lastPtr > 0x80)
        lastPtr = 0x80;
    *(uint32_t *)(bandArray->prev + 4) = lastPtr;
    for (si=0; si < lastPtr; si++) {
        *out = *in;
        out++;
        in++;
    }

    // Compress the data
    do {
        max = endIn - in > 0x202 ? 0x202 : endIn - in;

        if (!max) {
            if (rawDataNr)
                *rawDataPtr = rawDataNr - 1;
            bandArray->next = out;
            return 0;
        } else if (max >= 2) {
            maxRepCnt = 0;
            maxPtr = 0;

            // Check the best similar piece of data
            for (i=0; i < 0x40; i++) {
                unsigned char *seq = in + ptrArray[i] + 1;

                if (seq < beginIn)
                    continue;
                if (in <= seq)
                    continue;
                for (repCnt = 0; repCnt < max && repCnt < maxSizeArray[i]; 
                        repCnt++)
                    if (in[repCnt] != seq[repCnt])
                        break;
                if (repCnt > maxRepCnt) {
                    maxRepCnt = repCnt;
                    maxPtr = i;
                }
            }

            // If the piece is large enough, use it!
            if (maxRepCnt > 2) {
                maxRepCnt -= 3;
                out[0] = 0x80 | maxRepCnt & 0x7F;
                out[1] = ((maxRepCnt >> 1) & 0xC0) | maxPtr & 0x3F;
                out += 2;
                in += maxRepCnt + 3;
                if (rawDataNr) {
                    *rawDataPtr = rawDataNr - 1;
                    rawDataNr = 0;
                }
                continue;
            }
        }

        // Write the uncompressed data
        rawDataNr++;
        if (rawDataNr == 1) {
            rawDataPtr = out;
            out++;
        } else if (rawDataNr == 0x80) {
            *rawDataPtr = 0x7F;
            rawDataNr = 0;
        }
        *out = *in;
        out++;
        in++;

    } while (out <= endOut);

    return -1;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin: */

