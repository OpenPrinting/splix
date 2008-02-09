/*
 *      algo0x11.cpp            (C) 2006-2007, Aurélien Croc (AP²C)
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
#include "algo0x11.h"
#include <QtCore/QByteArray>
#include <QtCore/QTextStream>
#include "page.h"
#include "i18n.h"


bool decompress0x11(const QByteArray& input, QByteArray& output, 
    quint16 width, quint16 height, Page* page, QTextStream& err)
{
    quint32 lastOccur, bandSize, idx=4, w=0;
    short occurTable[0x40];
    QByteArray data;

    // Check the size
    if (input.size() < 0x40*2 + 4) {
        err << _("Algo0x11: bad band size") << endl;
        return false;
    }

    // Prepare the band buffer
    bandSize = (width * height + 7) >> 3;
    data.fill(0xff, bandSize);

    // Extract the header data and the occurrence table
    lastOccur = page->read32(input, 0);
    for (unsigned int i=0; i < 0x40; i++)
        occurTable[i] = ~page->read16(input, idx + i*2);
    idx += 0x40 * 2;

    // Extract the first uncompressed data
    if (input.size() <= idx + lastOccur) {
        err << _("Algo0x11: invalid band size") << endl;
        return false;
    }
    for (w=0; w < lastOccur; w++)
        data[w] = page->read8(input, idx + w);
    idx += lastOccur;

    // Decompress the other data
    while (input.size() > idx) {
        quint8 counter = page->read8(input, idx++);

        // Compressed
        if (counter & 0x80) {
            quint8 number = page->read8(input, idx++);
            quint32 toRead, ref;

            toRead = ((number & 0xC0) << 1) + (counter & 0x7F) + 3;
            number = number & ~0xC0;
            ref = w + 1;
            for (unsigned int i=0; i < toRead; i++, w++)
                data[w] = data.at(ref + occurTable[number] + i);

        // Uncompressed
        } else {
            for (unsigned int i = 0; i <= counter; i++, w++)
                data[w] = page->read8(input, idx++);
        }
    }

    // Rotate the data
    output.fill(0, bandSize);
    for (unsigned int i = 0; i < bandSize; i++) {
        uint32_t x, y;

        x = i / height;
        y = i % height;
        output[x + y * (width / 8)] = ~data[i];
    }

    return true;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8 : */

