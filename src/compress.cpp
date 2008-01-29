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
#include <string.h>
#include "page.h"
#include "band.h"
#include "errlog.h"
#include "request.h"
#include "bandplane.h"

#include "algo0x11.h"

static bool _isEmptyBand(unsigned char* band, unsigned long size)
{
    unsigned long max = size / sizeof(unsigned long);

    for (unsigned int i=0; i < max; i++) {
        if (((unsigned long*)band)[i])
            return false;
    }
    for (unsigned int i=0; i < size & 0x3; i++)
        if (band[size-i-1])
            return false;
    return true;
}

static bool _compressBandedPage(const Request& request, Page* page)
{
    unsigned long index=0, pageHeight, lineWidthInB, bandHeight, bandSize;
    unsigned char *planes[4], *band;
    unsigned long bandNumber=1;
    unsigned char colors;

    colors = page->colorsNr();
    lineWidthInB = (page->width() + 7) / 8;
    pageHeight = page->height();
    bandHeight = request.printer()->bandHeight();
    bandSize = lineWidthInB * bandHeight;
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
        unsigned long bytesToCopy = bandSize;
        Band *current = NULL;
        bool theEnd = false;

        // Special things to do for the last band
        if (pageHeight < bandHeight) {
            theEnd = true;
            bytesToCopy = lineWidthInB * pageHeight;
            memset(band, 0, bandSize);
        }

        for (unsigned int i=0; i < colors; i++) {
            BandPlane *plane;
            Algo0x11 algo;

            memcpy(band, planes[i] + index, bytesToCopy);
/*            if (_isEmptyBand(band, bandSize)) {
                DEBUGMSG("VIDE ?");
                continue;
            }*/
            plane = algo.compress(request, band, page->width(), bandHeight);
            if (plane) {
                plane->setColorNr(i+1);
                if (!current)
                    current = new Band(bandNumber, page->width(), bandHeight);
                current->registerPlane(plane);
            }
        }
        if (current)
            page->registerBand(current);
        bandNumber++;
        index += bandSize;
        pageHeight = theEnd ? 0 : pageHeight - bandHeight;
    }
    page->flushPlanes();
    delete[] band;

    return true;
}

bool compressPage(const Request& request, Page* page)
{
    switch(page->compression()) {
        case 0x11:
            return _compressBandedPage(request, page);
        case 0x13:
#ifndef DISABLE_JBIG
            /* TODO TODO TODO */
            break;
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

