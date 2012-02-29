/*
 * 	    colors.h                  (C) 2006-2008, Aurélien Croc (AP²C)
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
#ifndef _COLORS_H_
#define _COLORS_H_

#ifndef DISABLE_BLACKOPTIM

class Page;

/**
  * Optimize the black channel.
  * Transform red, green and cyan dots in a black dot and remove red, green or
  * cyan dot if a black dot is present.
  */
extern void applyBlackOptimization(Page* page);

#endif /* DISABLE_BLACKOPTIM */
#endif /* _COLORS_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

