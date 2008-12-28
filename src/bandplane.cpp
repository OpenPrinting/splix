/*
 * 	    bandplane.cpp             (C) 2006-2008, Aurélien Croc (AP²C)
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
#include "bandplane.h"
#include <unistd.h>

/*
 * Constructeur - Destructeur
 * Init - Uninit 
 */
BandPlane::BandPlane()
{
    _endian = Dependant;
    _size = 0;
    _data = NULL;
}

BandPlane::~BandPlane()
{
    if (_data)
        delete[] _data;
}


/*
 * Enregistrement des données
 * Set data
 */
void BandPlane::setData(unsigned char *data, unsigned long size)
{
    if (!data)
        size = 0;
    if (_data)
        delete[] _data;

    _data = data;
    _size = size;
    _checksum = 0;
    for (unsigned int i=0; i < _size; i++)
        _checksum += (unsigned char)_data[i];
}



/*
 * Mise sur disque / Rechargement
 * Swapping / restoring
 */
bool BandPlane::swapToDisk(int fd)
{
    write(fd, &_colorNr, sizeof(_colorNr));
    write(fd, &_size, sizeof(_size));
    write(fd, _data, _size);
    write(fd, &_checksum, sizeof(_checksum));
    write(fd, &_endian, sizeof(_endian));
    write(fd, &_compression, sizeof(_compression));
    return true;
}

BandPlane* BandPlane::restoreIntoMemory(int fd)
{
    unsigned char* data;
    BandPlane* plane;

    plane = new BandPlane();
    read(fd, &plane->_colorNr, sizeof(plane->_colorNr));
    read(fd, &plane->_size, sizeof(plane->_size));
    data = new unsigned char[plane->_size];
    read(fd, data, plane->_size);
    plane->_data = data;
    read(fd, &plane->_checksum, sizeof(plane->_checksum));
    read(fd, &plane->_endian, sizeof(plane->_endian));
    read(fd, &plane->_compression, sizeof(plane->_compression));

    return plane;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

