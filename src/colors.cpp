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
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  $Id$
 * 
 */
#include "colors.h"
#include "page.h"

#ifndef DISABLE_BLACKOPTIM
void applyBlackOptimization(Page* page)
{
    // Only optimize colors if there are 4 (CMYK)
    if (!page || page->colorsNr() != 4)
        return;

    std::array<std::span<uint8_t>, 4> planes;
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t* ptr = page->planeBuffer(i);
        if (!ptr) return;
        planes[i] = std::span<uint8_t>(ptr, page->width() * page->height() / 8);
    }

    const size_t size = planes[0].size();
    
    // Process in chunks of unsigned long for performance where possible, 
    // but use a safer approach than raw reinterpret_cast if we were strictly 
    // following C++23, however for high-performance bitwise ops on buffers 
    // we'll use a clean loop that the compiler can optimize.
    
    for (size_t i = 0; i < size; ++i) {
        uint8_t& cyan = planes[0][i];
        uint8_t& magenta = planes[1][i];
        uint8_t& yellow = planes[2][i];
        uint8_t& black = planes[3][i];

        // optimization 1: if black is present, clear CMY
        if (black) {
            cyan &= ~black;
            magenta &= ~black;
            yellow &= ~black;
        }

        // optimization 2: if all CMY present, convert to black and clear CMY
        uint8_t common = cyan & magenta & yellow;
        if (common) {
            black |= common;
            cyan &= ~common;
            magenta &= ~common;
            yellow &= ~common;
        }
    }
}

#endif /* DISABLE_BLACKOPTIM */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

