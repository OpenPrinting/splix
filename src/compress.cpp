/*
 * 	    compress.cpp              (C) 2006-2008, Aurélien Croc (AP²C)
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
#include <math.h>
#include <string.h>
#include "page.h"
#include "band.h"
#include "errlog.h"
#include "request.h"
#include "bandplane.h"

#include "algo0x11.h"
#include "algo0x13.h"

static bool _isEmptyBand(unsigned char* band, unsigned long size)
{
    unsigned long max, mod;

    max = size / sizeof(unsigned long);
    mod = size % sizeof(unsigned long);

    for (unsigned long i=0; i < max; i++) {
        if (((unsigned long*)band)[i])
            return false;
    }
    for (unsigned long i=0; i < mod; i++)
        if (band[size-i-1])
            return false;
    return true;
}

static bool _compressBandedPage(const Request& request, Page* page)
{
    unsigned long index=0, pageHeight, pageWidth, lineWidthInB, bandHeight;
    unsigned long bandSize, hardMarginX, hardMarginXInB, hardMarginY;
    unsigned char *planes[4], *band;
    unsigned long bandNumber=0;
    unsigned char colors;

    colors = page->colorsNr();
    hardMarginX = ((unsigned long)ceill(page->convertToXResolution(request.
        printer()->hardMarginX())) + 7) & ~7;
    hardMarginY = ceill(page->convertToYResolution(request.printer()->
        hardMarginY()));
    hardMarginXInB = hardMarginX / 8;
    pageWidth = page->width() - hardMarginX;
    pageHeight = page->height() - hardMarginY;
    page->setWidth(pageWidth);
    page->setHeight(pageHeight);
    lineWidthInB = (pageWidth + 7) / 8;
    bandHeight = request.printer()->bandHeight();
    bandSize = lineWidthInB * bandHeight;
    index = hardMarginY * (lineWidthInB + hardMarginXInB);
    band = new unsigned char[bandSize];
    for (unsigned int i=0; i < colors; i++)
        planes[i] = page->planeBuffer(i);

    /*
     * 1. On vérifier si la bande n'est pas blanche
     *      |-> Si bande blanche, on passe
     *      '-> Sinon, on compresse
     * 2. On rajoute les informations de bande (numéro de bande et de
     *    couleur).
     * 3. On enregistre la bande dans la page.
     * 4. On détruit les buffers de plans dans la page.
     */
    while (pageHeight) {
        unsigned long localHeight = bandHeight;
        Band *current = NULL;
        bool theEnd = false;

        // Special things to do for the last band
        if (pageHeight < bandHeight) {
            theEnd = true;
            localHeight = pageHeight;
            memset(band, 0, bandSize);
        }

        for (unsigned int i=0; i < colors; i++) {
            BandPlane *plane;
            Algo0x11 algo;

            // Copy the data into the band depending on the algorithm options
            if (algo.reverseLineColumn()) {
                for (unsigned int y=0; y < localHeight; y++) {
                    for (unsigned int x=0; x < lineWidthInB; x++) {
                            band[x * bandHeight + y] = planes[i][index + x +
                                hardMarginXInB + y * (lineWidthInB + 
                                hardMarginXInB)];
                    }
                }
            } else {
                for (unsigned int y=0; y < localHeight; y++) {
                    for (unsigned int x=0; x < lineWidthInB; x++) {
                            band[x + y * lineWidthInB] = planes[i][index + x +
                                hardMarginXInB + y * (lineWidthInB + 
                                hardMarginXInB)];
                    }
                }
            }

            // Does the band is empty?
             if (_isEmptyBand(band, bandSize))
                 continue;

            // Check if bytes have to be reversed
            if (algo.inverseByte())
                for (unsigned int j=0; j < bandSize; j++)
                    band[j] = ~band[j];

            // Call the compression method
            plane = algo.compress(request, band, pageWidth, bandHeight);
            if (plane) {
                plane->setColorNr(i + 1);
                if (!current)
                    current = new Band(bandNumber, pageWidth, bandHeight);
                current->registerPlane(plane);
            }
        }
        if (current)
            page->registerBand(current);
        bandNumber++;
        index += bandSize + localHeight * hardMarginXInB;
        pageHeight = theEnd ? 0 : pageHeight - bandHeight;
    }
    page->flushPlanes();
    delete[] band;

    return true;
}

#ifndef DISABLE_JBIG
static bool _compressWholePage(const Request& request, Page* page)
{
    unsigned long hardMarginX, hardMarginXInB, hardMarginY, lineWidthInB;
    unsigned long pageWidth, bandHeight, planeHeight, pageHeight, index;
    unsigned long bandNumber=0;
    unsigned char *buffer;
    Band *current = NULL;
    Algo0x13 algo[4];

    hardMarginX = ((unsigned long)ceill(page->convertToXResolution(request.
        printer()->hardMarginX())) + 7) & ~7;
    hardMarginY = ceill(page->convertToYResolution(request.printer()->
        hardMarginY()));
//    hardMarginX = hardMarginY = 0;
    hardMarginXInB = hardMarginX / 8;
    pageWidth = page->width() - hardMarginX * 2;
    pageHeight = page->height() - hardMarginY * 2;
    page->setWidth(pageWidth);
    page->setHeight(pageHeight);
    lineWidthInB = (pageWidth + 7) / 8;
    bandHeight = request.printer()->bandHeight();
    // Alignment of the page height on band height
    planeHeight = ((pageHeight + bandHeight - 1) / bandHeight) * bandHeight;
    buffer = new unsigned char[lineWidthInB * planeHeight];

    do {
        current = NULL;
        for (unsigned int i=0; i < page->colorsNr(); i++) {
            unsigned char *curPlane = page->planeBuffer(i);
            BandPlane *plane;

            index = hardMarginY * (lineWidthInB + 2 * hardMarginXInB) + 
                hardMarginXInB;
            for (unsigned int y=0; y < pageHeight; y++, 
                    index += 2 * hardMarginXInB) {
                for (unsigned int x=0; x < lineWidthInB; x++, index++) {
                    buffer[x + y * lineWidthInB] = curPlane[index];
                }
            }

            // Call the compression method
            plane = algo[i].compress(request, buffer, page->width(), 
                planeHeight);
            if (plane) {
                plane->setColorNr(i + 1);
                if (!current)
                    current = new Band(bandNumber, page->width(), 
                        request.printer()->bandHeight());
                current->registerPlane(plane);
            }
        }
        if (current)
            page->registerBand(current);
        bandNumber++;
    } while (current);

    page->flushPlanes();
    delete[] buffer;

    return true;
}
#endif /* DISABLE_JBIG */

bool compressPage(const Request& request, Page* page)
{
    switch(page->compression()) {
        case 0x11:
            return _compressBandedPage(request, page);
        case 0x13:
#ifndef DISABLE_JBIG
            return _compressWholePage(request, page);
#else
            ERRORMSG(_("J-BIG compression algorithm has been disabled during "
                "the compilation. Please recompile SpliX and enable the "
                "J-BIG compression algorithm."));
            break;
#endif /* DISABLE_JBIG */
        default:
            ERRORMSG(_("Compression algorithm 0x%lX does not exist"), 
                page->compression());
    }
    return false;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

