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
 *  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  $Id$
 * 
 */
#include "algo0x11.h"
#include <string.h>
#include <stdlib.h>
#include "bandplane.h"
#include "errlog.h"



/*
 * Fonctions locales
 * Local functions
 */
int Algo0x11::__compare(const void *n1, const void *n2)
{
    // n2 and n1 has been exchanged since the first
    // element of the array MUST be the biggest
    return *(uint32_t *)n2 - *(uint32_t *)n1;
}

bool Algo0x11::_lookupBestOccurs(const unsigned char* data, unsigned long size)
{
    uint32_t occurs[COMPRESS_SAMPLE_RATE][2];
    bool oneIsPresent = false;
    unsigned char b;
    unsigned long i;

    // Initialize the table
    // occurs[i][0] = number of occurences of the offset
    // occurs[i][1] = offset
    for (i=0; i < COMPRESS_SAMPLE_RATE; i++) {
        occurs[i][0] = 0;
        occurs[i][1] = i;
    }

    // Calculate the byte occurrence
    for (i=COMPRESS_SAMPLE_RATE; i < size; i += COMPRESS_SAMPLE_RATE) {
        b = data[i];
        for (unsigned long j=1; j < COMPRESS_SAMPLE_RATE; j++)
            if (data[i - j] == b)
                occurs[(j - 1)][0]++;
    }

    // Order the array
    qsort(occurs, COMPRESS_SAMPLE_RATE, sizeof(uint32_t)*2, __compare);

    // Set the pointer table to use for compression
    for (i=0; i < TABLE_PTR_SIZE; i++) {
        _ptrArray[i] = occurs[i][1] + 1;
        if (_ptrArray[i] == 1)
            oneIsPresent = true;
    }
    // Append the value 1 which improves the compression of multiple same bytes
    if (!oneIsPresent)
        _ptrArray[TABLE_PTR_SIZE-1] = 1;

    return true;
}

bool Algo0x11::_compress(const unsigned char *data, unsigned long size, 
    unsigned char* &output, unsigned long &outputSize)
{
    unsigned long r, w=4, uncompSize=0, maxCompSize, bestCompCounter, bestPtr;
    unsigned long rawDataCounter = 0, rawDataCounterPtr=0, maxOutputSize;
    unsigned char *out;

    // Create the output buffer
    maxOutputSize = size;
    out = new unsigned char[maxOutputSize];

    // Print the table
    for (unsigned long i=0; i < TABLE_PTR_SIZE; i++, w += 2) {
        *(uint16_t *)(out + w) = (uint16_t)_ptrArray[i];
        if (_ptrArray[i] > uncompSize)
            uncompSize = _ptrArray[i];
    }

    // Print the first uncompressed bytes
    if (uncompSize > MAX_UNCOMPRESSED_BYTES)
        uncompSize = MAX_UNCOMPRESSED_BYTES;
    *(uint32_t *)out = (uint32_t)uncompSize;
    for (r=0; r < uncompSize; r++, w++)
        out[w] = data[r];

    //
    // Compress the data
    //
    do {
        maxCompSize = size - r > MAX_COMPRESSED_BYTES ? MAX_COMPRESSED_BYTES :
            size - r;

        // End of the compression
        if (!maxCompSize) {
            if (rawDataCounter)
                out[rawDataCounterPtr] = rawDataCounter - 1;
            break;

        // Try to compress the next piece of data
        } else if (maxCompSize >= 2) {
            bestCompCounter = 0;
            bestPtr = 0;

            // Check if there is enough space
            if (w + 2 >= maxOutputSize) {
                w += 2;
                break;
            }

            // Check the best similar piece of data
            for (unsigned long i=0; i < TABLE_PTR_SIZE; i++) {
                unsigned long rTmp, counter;
               
                if (_ptrArray[i] > r)
                    continue;
                rTmp = r - _ptrArray[i];
                for (counter = 0; counter < maxCompSize; counter++)
                    if (data[r + counter] != data[rTmp + counter])
                        break;
                if (counter > bestCompCounter) {
                    bestCompCounter = counter;
                    bestPtr = i;
                }
            }

            // If the reproduced piece is large enough, use it!
            if (bestCompCounter > MIN_COMPRESSED_BYTES) {
                r += bestCompCounter;
                bestCompCounter -= 3;
                out[w] = COMPRESSION_FLAG | (bestCompCounter & 0x7F);
                out[w+1] = ((bestCompCounter >> 1) & 0xC0) | (bestPtr & 0x3F);
                w += 2;
                if (rawDataCounter) {
                    out[rawDataCounterPtr] = rawDataCounter - 1;
                    rawDataCounter = 0;
                }
                continue;
            }
        }

        // Else write the uncompressed data
        rawDataCounter++;
        if (rawDataCounter == 1) {
            // Check if there is enough space
            if (w + 2 >= maxOutputSize) {
                w += 2;
                break;
            }
            rawDataCounterPtr = w;
            w++;
        } else if (rawDataCounter == MAX_UNCOMPRESSED_BYTES) {
            out[rawDataCounterPtr] = 0x7F;
            rawDataCounter = 0;
        }
        out[w] = data[r];
        w++;
        r++;
    } while (w < maxOutputSize);

    // Does the compression finished without any error?
    if (w >= maxOutputSize) {
        ERRORMSG(_("No more space available in the output buffer for "
            "compression"));
        delete[] out;
        return false;
    }

    // Copy the buffer in a best buffer
    outputSize = w;
    output = new unsigned char[outputSize];
    memcpy(output, out, outputSize);
    delete[] out;

    return true;
}




/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
Algo0x11::Algo0x11()
{
}

Algo0x11::~Algo0x11()
{
}



/*
 * Routine de compression
 * Compression routine
 */
BandPlane* Algo0x11::compress(const Request& request, unsigned char *data, 
        unsigned long width, unsigned long height)
{
    unsigned long outputSize, size = width * height / 8;
    unsigned char *output;
    BandPlane *plane;

    if (!data || !size) {
        ERRORMSG(_("Invalid given data for compression (0x11)"));
        return NULL;
    }

    // Lookup for the best occurs
    if (!_lookupBestOccurs(data, size) || 
        !_compress(data, size, output, outputSize)) {
        return NULL;
    }

    // Register the result into a band plane
    plane = new BandPlane();
    plane->setData(output, outputSize);
    plane->setEndian(BandPlane::Dependant);
    plane->setCompression(0x11);

    return plane;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

