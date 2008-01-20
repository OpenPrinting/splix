/*
 * 	    compress.cpp              (C) 2006-2007, Aurélien Croc (AP²C)
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
#include "errlog.h"
#include "request.h"

static bool _compressBandedPage(const Request& request, Page& page)
{
    unsigned long index, pageHeight, lineWidthInB, bandHeight, bandSize;
    unsigned char *planes[4], *band;
    unsigned long bandNumber=1;
    unsigned char colors;

    colors = page.colorsNr();
    lineWidthInB = (page.width() + 7) / 8;
    bandHeight = request.printer()->bandHeight();
    bandSize = lineWidthInB * bandHeight;
    band = new unsigned char[bandSize];
    for (unsigned int i=0; i < colors; i++)
        planes[i] = page.planeBuffer(i);

    while (pageHeight > bandHeight) {
        for (unsigned int i=0; i < colors; i++) {
            memcpy(band, planes[i] + index, bandSize);
            /*
             * 1. On vérifier si la bande n'est pas blanche
             *      |-> Si bande blanche, on passe
             *      '-> Sinon, on compresse
             * 2. On rajoute les informations de bande (numéro de bande et de
             *    couleur).
             * 3. On enregistre la bande dans la page.
             * 4. On détruit les buffers de plans dans la page.
             */
        }
        bandNumber++;
        index += bandSize;
        pageHeight -= bandHeight;
    }

    return true;
}

bool compressPage(const Request& request, Page& page)
{
    switch(page.compression()) {
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
            ERRORMSG(_("No compression algorithm known for 0x%lX"), 
                page.compression());
    }
    return false;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

