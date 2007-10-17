/*
 *      algos.cpp               (C) 2006-2007, Aurélien Croc (AP²C)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *   $Id$
 */
#include "page.h"
#include <QtCore/QByteArray>
#include <QtCore/QTextStream>
#include "i18n.h"



/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
Page::Page()
{
}



/*
 * Nettoyage
 * Clear all
 */
void Page::clear()
{
}



/* 
 * Réception d'une bande pour traitement
 * Manage band data
 */
bool Page::process(quint8 color, quint16 width, quint16 height, 
    quint8 compression, QByteArray& content, QTextStream& out, QTextStream& err)
{
    // Check if the compression is supported
    if (compression != 0x13) {
        err << QString(_("Page: Unsupported compression algorithm (0x%1)")).
            arg(compression, 0, 16) << endl;
        return false;
    }

    // Check the signature
    if ((quint8)content.at(0) != 0x39 || (quint8)content.at(1) != 0xAB || 
        (quint8)content.at(2) != 0xCD || (quint8)content.at(3) != 0xEF) {
        err << QString(_("Page: Invalid signature")) << endl;
        return false;
    }


    return true;
}

void Page::flush()
{
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8 : */

