/*
 * 	    algo0x11.cpp              (C) 2006-2007, Aurélien Croc (AP²C)
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
#include "algo0x11.h"
#include "bandplane.h"
#include <string.h>

/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
Algo0x11::Algo0x11()
{
}

Algo0x11::~Algo0x11()
{
}



/*
 * Routine de compression
 * Compression routine
 */
BandPlane* Algo0x11::compress(const Request& request, unsigned char *data, 
        unsigned long width, unsigned long height)
{
    BandPlane *plane;
    unsigned char *buffer;

    buffer = new unsigned char[width * height / 8];
    memcpy(buffer, data, width*height / 8);
    plane = new BandPlane();
    plane->setData(buffer);
    plane->setDataSize(width * height / 8);
    return plane;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

