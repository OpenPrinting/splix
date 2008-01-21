/*
 * 	    band.cpp                  (C) 2006-2007, Aurélien Croc (AP²C)
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
#include "band.h"
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


/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

