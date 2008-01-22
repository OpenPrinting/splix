/*
 * 	    printer.cpp               (C) 2006-2008, Aurélien Croc (AP²C)
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
#include "printer.h"
#include "request.h"
#include "ppdfile.h"

/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
Printer::Printer()
{
    _manufacturer = NULL;
    _model = NULL;
}

Printer::~Printer()
{
    if (_manufacturer)
        delete[] _manufacturer;
    if (_model)
        delete [] _model;
}



/*
 * Chargement des informations sur l'imprimante
 * Load the printer information
 */
bool Printer::loadInformation(const Request& request)
{
    _manufacturer = request.ppd()->get("Manufacturer").deepCopy();
    _model = request.ppd()->get("ModelName").deepCopy();

    // XXX XXX XXX XXX
    _bandHeight = 0x80;

    return true;
}


/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

