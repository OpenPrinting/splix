/*
 * 	bandanalyser.cpp		(C) 2006, Aurélien Croc (AP²C)
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
#include "bandanalyser.h"
#include "band.h"
#include "error.h"


/*
 * Vérification si les bandes sont vides
 * Check if bands are empty
 */
void checkEmptyBand(Band *band)
{
	const unsigned char *data;
	unsigned long i;
	size_t size, last;

	data = band->band();
	size = (band->width() * band->height() + 7) >> 3;
	last = size % sizeof(unsigned long);
	size -= last;

	for (i=0; i < size; i += sizeof(unsigned long))
	       if (~*((unsigned long *)&data[i]) != 0) {
		       return;
	       }
	for (; i < last + size; i++)
		if (~data[i] != 0)
			return;
	band->setEmpty();
}

void correctBlackColor(Band *bandC, Band *bandM, Band *bandY, Band *bandB)
{
	unsigned char *cyan, *magenta, *yellow, *black;
	unsigned long i;
	size_t size, last;

	cyan = bandC->band();
	magenta = bandM->band();
	yellow = bandY->band();
	black = bandB->band();

	size = (bandC->width() * bandC->height() + 7) >> 3;
	last = size % sizeof(unsigned long);
	size -= last;

	for (i=0; i < size; i += sizeof(unsigned long)) {
		unsigned long mask;

		mask = *((unsigned long *)&cyan[i]) | 
			*((unsigned long *)&magenta[i]) | 
			*((unsigned long *)&yellow[i]);
		if (~mask == 0)
			continue;
		
		*((unsigned long *)&cyan[i]) |= ~mask;
		*((unsigned long *)&magenta[i]) |= ~mask;
		*((unsigned long *)&yellow[i]) |= ~mask;
		*((unsigned long *)&black[i]) &= mask;
	}
	
	for (; i < last + size; i++) {
		unsigned char mask;

		mask = cyan[i] | magenta[i] | yellow[i];
		if (mask == 0xFF)
			continue;
		cyan[i] |= ~mask;
		magenta[i] |= ~mask;
		yellow[i] |= ~mask;
		black[i] &= mask;
	}
}
