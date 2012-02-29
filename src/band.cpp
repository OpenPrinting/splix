/*
 * 	    band.cpp                  (C) 2006-2008, Aurélien Croc (AP²C)
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
#include "band.h"
#include <unistd.h>
#include "errlog.h"
#include "bandplane.h"

/*
 * Constructeur - Destructeur
 * Init - Uninit 
 */
Band::Band()
{
    _bandNr = 0;
    _colors = 0;
    _parent = NULL;
    _planes[0] = NULL;
    _planes[1] = NULL;
    _planes[2] = NULL;
    _planes[3] = NULL;
    _width = 0;
    _height = 0;
    _sibling = NULL;
}

Band::Band(unsigned long nr, unsigned long width, unsigned long height)
{
    _colors = 0;
    _parent = NULL;
    _planes[0] = NULL;
    _planes[1] = NULL;
    _planes[2] = NULL;
    _planes[3] = NULL;
    _sibling = NULL;
    _bandNr = nr;
    _width = width;
    _height = height;
}

Band::~Band()
{
    for (unsigned int i=0; i < _colors; i++)
        delete _planes[i];
    if (_sibling)
        delete _sibling;
}



/*
 * Mise sur disque / Rechargement
 * Swapping / restoring
 */
bool Band::swapToDisk(int fd)
{
    write(fd, &_bandNr, sizeof(_bandNr));
    write(fd, &_colors, sizeof(_colors));
    write(fd, &_width, sizeof(_width));
    write(fd, &_height, sizeof(_height));
    for (unsigned int i=0; i < _colors; i++)
        if (!_planes[i]->swapToDisk(fd))
            return false;
    return true;
}

Band* Band::restoreIntoMemory(int fd)
{
    unsigned char colors;
    Band* band;

    band = new Band();
    read(fd, &band->_bandNr, sizeof(band->_bandNr));
    read(fd, &colors, sizeof(colors));
    read(fd, &band->_width, sizeof(band->_width));
    read(fd, &band->_height, sizeof(band->_height));
    for (unsigned int i=0; i < colors; i++) {
        BandPlane *plane = BandPlane::restoreIntoMemory(fd);
        if (!plane) {
            delete band;
            return NULL;
        }
        band->registerPlane(plane);
    }

    return band;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

