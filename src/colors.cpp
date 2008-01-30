/*
 * 	    colors.cpp                (C) 2006-2008, Aurélien Croc (AP²C)
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
#include "colors.h"
#include "page.h"

#ifndef DISABLE_BLACKOPTIM
void applyBlackOptimization(Page* page)
{
    unsigned long size, sizeByUL, mod, mask;
    unsigned char *planes[4], bmask;

    // Only optimize colors if there are present
    if (!page || page->colorsNr() != 4)
        return;
    for (unsigned int i=0; i < 4; i++) {
        if (!(planes[i] = page->planeBuffer(i)))
            return;
    }

    size = page->width() * page->height() / 8;
    sizeByUL = size / sizeof(unsigned long);
    mod = size % sizeof(unsigned long);


    /*
     * To optimize this algorithm, data are first evaluated by unsigned long
     * (32-Bits on 32-Bits architecture and 64-Bits on 64-Bits architecture).
     * The last bytes are evaluated individually if the size is not a multiple
     * of the size of the unsigned long
     */
    for (unsigned long i=0; i < sizeByUL; i++) {
        // Clear cyan, magenta and yellow dots if a black dot is present
        mask = ((unsigned long *)planes[0])[i];
        if (mask) {
            ((unsigned long *)planes[1])[i] &= ~mask;
            ((unsigned long *)planes[2])[i] &= ~mask;
            ((unsigned long *)planes[3])[i] &= ~mask;
        }

        // Set a black dot if cyan, magenta and yellow dots are present and
        // clear them
        mask = ((unsigned long *)planes[1])[i];
        mask &= ((unsigned long *)planes[2])[i];
        mask &= ((unsigned long *)planes[3])[i];
        if (mask) {
            ((unsigned long *)planes[0])[i] |= mask;
            ((unsigned long *)planes[1])[i] &= ~mask;
            ((unsigned long *)planes[2])[i] &= ~mask;
            ((unsigned long *)planes[3])[i] &= ~mask;
        }
    }

    for (unsigned long i=1; i <= mod; i++) {
        // Clear cyan, magenta and yellow dots if a black dot is present
        bmask = planes[0][size - i];
        if (bmask) {
            planes[1][size - i] &= ~bmask;
            planes[2][size - i] &= ~bmask;
            planes[3][size - i] &= ~bmask;
        }

        // Set a black dot if cyan, magenta and yellow dots are present and
        // clear them
        bmask = planes[1][size - i];
        bmask &= planes[2][size - i];
        bmask &= planes[3][size - i];
        if (bmask) {
            planes[0][size - i] |= bmask;
            planes[1][size - i] &= ~bmask;
            planes[2][size - i] &= ~bmask;
            planes[3][size - i] &= ~bmask;
        }
    }
}

#endif /* DISABLE_BLACKOPTIM */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

