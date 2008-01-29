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
#include <stddef.h>

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


/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

