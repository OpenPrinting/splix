/*
 *      algo0x11.h              (C) 2006-2007, Aurélien Croc (AP²C)
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *   $Id$
 */
#ifndef _ALGO0X11_H_
#define _ALGO0X11_H_

#include <QtCore/qglobal.h>

class Page;
class QByteArray;
class QTextStream;

extern bool decompress0x11(const QByteArray& input, QByteArray& output, 
    quint16 width, quint16 height, Page* page, QTextStream& err);

#endif /* _ALGO0X11_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8 : */

