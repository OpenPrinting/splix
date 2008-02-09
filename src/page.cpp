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
#include <unistd.h>
#include "band.h"
#include "errlog.h"

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



/*
 * Mise sur disque / Rechargement
 * Swapping / restoring
 */
bool Page::swapToDisk(int fd)
{
    unsigned long i;
    Band* band;

    if (_planes[0] || _planes[1] || _planes[2] || _planes[3]) {
        ERRORMSG(_("Cannot swap page instance which still contains bitmap "
            "representation"));
        return false;
    }
    write(fd, &_xResolution, sizeof(_xResolution));
    write(fd, &_yResolution, sizeof(_yResolution));
    write(fd, &_width, sizeof(_width));
    write(fd, &_height, sizeof(_height));
    write(fd, &_colors, sizeof(_colors));
    write(fd, &_pageNr, sizeof(_pageNr));
    write(fd, &_copiesNr, sizeof(_copiesNr));
    write(fd, &_compression, sizeof(_compression));
    write(fd, &_empty, sizeof(_empty));
    write(fd, &_bandsNr, sizeof(_bandsNr));
    for (i=0, band = _firstBand; i < _bandsNr; i++) {
        if (!band->swapToDisk(fd))
            return false;
        band = band->sibling();
    }

    return true;
}

Page* Page::restoreIntoMemory(int fd)
{
    unsigned long nr;
    Page* page;

    page = new Page();
    read(fd, &page->_xResolution, sizeof(page->_xResolution));
    read(fd, &page->_yResolution, sizeof(page->_yResolution));
    read(fd, &page->_width, sizeof(page->_width));
    read(fd, &page->_height, sizeof(page->_height));
    read(fd, &page->_colors, sizeof(page->_colors));
    read(fd, &page->_pageNr, sizeof(page->_pageNr));
    read(fd, &page->_copiesNr, sizeof(page->_copiesNr));
    read(fd, &page->_compression, sizeof(page->_compression));
    read(fd, &page->_empty, sizeof(page->_empty));
    read(fd, &nr, sizeof(nr));
    for (unsigned int i=0; i < nr; i++) {
        Band *band = Band::restoreIntoMemory(fd);
        if (!band) {
            delete page;
            return NULL;
        }
        page->registerBand(band);
    }

    return page;
}


/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

