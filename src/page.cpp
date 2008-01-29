/*
 * 	    page.cpp                  (C) 2006-2008, Aurélien Croc (AP²C)
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
#include "page.h"
#include "band.h"

/*
 * Constructeur - Destructeur
 * Init - Uninit 
 */
Page::Page()
{
    _empty = true;
    _xResolution = 0;
    _yResolution = 0;
    _planes[0] = NULL;
    _planes[1] = NULL;
    _planes[2] = NULL;
    _planes[3] = NULL;
    _firstBand = NULL;
    _lastBand = NULL;
}

Page::~Page()
{
    flushPlanes();
    if (_firstBand)
        delete _firstBand;
}



/*
 * Enregistrement d'une nouvelle bande
 * Register a new band
 */
void Page::registerBand(Band *band)
{
    if (_lastBand)
        _lastBand->registerSibling(band);
    else
        _firstBand = band;
    _lastBand = band;
    band->registerParent(this);
}


/*
 * Libération de la mémoire utilisée par les couches
 * Flush the planes
 */
void Page::flushPlanes()
{
    for (unsigned int i=0; i < 4; i++) {
        if (_planes[i]) {
            delete[] _planes[i];
            _planes[i] = NULL;
        }
    }
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

