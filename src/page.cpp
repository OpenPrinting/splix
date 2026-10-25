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
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  $Id$
 * 
 */
#include "page.h"
#include <unistd.h>
#include <string.h>
#include "band.h"
#include "errlog.h"
#include "io_utils.h"

/*
 * This magic formula reverse the bit of a byte. ie. the bit 1 becomes the 
 * bit 8, the bit 2 becomes the bit 7 etc.
 */
#define REVERSE_BITS(N) ((N * 0x0202020202ULL & 0x010884422010ULL) % 1023)

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
    _bandsNr = 0;
    _bih = NULL;
}

Page::~Page()
{
    flushPlanes();
    if (_firstBand)
        delete _firstBand;
    if (_bih)
        delete[] _bih;
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
    _bandsNr++;
}



/*
 * Rotation des couches
 * Rotate bitmaps planes
 */
void Page::rotate()
{
    unsigned long size, midSize;
    unsigned char tmp;

    size  = _width * _height / 8;
    midSize = size / 2;

    for (unsigned int i=0; i < _colors; i++) {
        for (unsigned long j=0; j < midSize; j++) {
            tmp = _planes[i][j];
            _planes[i][j] = REVERSE_BITS(_planes[i][size - j - 1]);
            _planes[i][size - j - 1] = REVERSE_BITS(tmp);
        }
    }
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
    _empty = false;
}



#include "page_io.inc"

void Page::setBIH(const unsigned char *bih_data) {
    if (NULL == _bih)
        _bih = new unsigned char[20];
    memcpy(_bih, bih_data, 20);
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

