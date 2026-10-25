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
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  $Id$
 * 
 */
#include "bandplane.h"
#include "io_utils.h"
#include <limits>
#include <new>
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
    return splix::writeAll(fd, &_colorNr, sizeof(_colorNr)) &&
        splix::writeAll(fd, &_size, sizeof(_size)) &&
        splix::writeAll(fd, _data, _size) &&
        splix::writeAll(fd, &_checksum, sizeof(_checksum)) &&
        splix::writeAll(fd, &_endian, sizeof(_endian)) &&
        splix::writeAll(fd, &_compression, sizeof(_compression));
}

BandPlane* BandPlane::restoreIntoMemory(int fd)
{
    unsigned char* data;
    BandPlane* plane;

    plane = new BandPlane();
    if (!splix::readAll(fd, &plane->_colorNr, sizeof(plane->_colorNr)) ||
        !splix::readAll(fd, &plane->_size, sizeof(plane->_size))) {
        delete plane;
        return NULL;
    }
    const std::size_t trailer = sizeof(plane->_checksum) +
        sizeof(plane->_endian) + sizeof(plane->_compression);
    if (plane->_size > std::numeric_limits<std::size_t>::max() - trailer ||
        !splix::canRead(fd, plane->_size + trailer)) {
        delete plane;
        return NULL;
    }
    data = new (std::nothrow) unsigned char[plane->_size];
    if (!data || !splix::readAll(fd, data, plane->_size)) {
        delete[] data;
        delete plane;
        return NULL;
    }
    plane->_data = data;
    if (!splix::readAll(fd, &plane->_checksum, sizeof(plane->_checksum)) ||
        !splix::readAll(fd, &plane->_endian, sizeof(plane->_endian)) ||
        !splix::readAll(fd, &plane->_compression, sizeof(plane->_compression))) {
        delete plane;
        return NULL;
    }

    return plane;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

