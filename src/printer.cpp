/*
 * 	    printer.cpp               (C) 2006-2007, Aurélien Croc (AP²C)
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

/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
Printer::Printer()
{
}

Printer::~Printer()
{
}



/*
 * Chargement des informations sur l'imprimante
 * Load the printer information
 */
bool Printer::loadInformation(const Request& request)
{
    ppd_attr_t *attr;
    ppd_file_t *ppd = request.ppd();

    attr = ppdFindAttr(ppd, "Manufacturer", NULL);
    if (attr)
        _manufacturer = attr->value;
    attr = ppdFindAttr(ppd, "ModelName", NULL);
    if (attr)
        _model = attr->value;


    return true;
}


/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

