/*
 * 	band.cpp		(C) 2006, Aurélien Croc (AP²C)
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
#include "error.h"
#include "compress.h"
#include <stdlib.h>
#include <string.h>


/*
 * Méthodes internes
 * Internal methods
 */
unsigned char *Band::_algorithm0(size_t *size)
{
	unsigned char *tmp;

	*size = _width * _height;
	tmp = new unsigned char[*size];
	memcpy(tmp, _band, _height * _width);
	return tmp;
}

unsigned char *Band::_algorithm11(size_t *size)
{
	struct BandArray band;
	unsigned char *output;

	output = new unsigned char[_width * _height + 8];
        band.array = output;
	band.next = output + 8;
	band.prev = output;
	*(uint32_t *)output = 0x09ABCDEF;

	calcOccurs(_band, _height, _width, 0x11);
	compressBand(&band, _band, _width, _height);
	*size = band.next - band.array;
	return band.array;
}

/*int Band::initCompression()
{
}*/



/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
Band::Band(unsigned long bandWidth, unsigned long bandHeight)
{
	_line = 0;
	_width = (bandWidth + 7) >> 3;
	_height = bandHeight;
	_band = NULL;
}

Band::~Band()
{
	if (_band)
		delete[] _band;
}



/*
 * Ajout d'une ligne
 * Add a line
 */
int Band::addLine(unsigned char *line, unsigned long width)
{
	int off = _line;

	if (!_line)
		_empty = false;
	if (!_band) {
		_band = new unsigned char[_width * _height];
		memset(_band, 0xFF, _width * _height);
	}

	// Clip the text
	line += _clipping;
	width -= _clipping;

	if (_line == _height) {
		ERROR(_("Band::addLine: the end of the band has been "
			"reached"));
		return -1;
	}

	for (unsigned int i=0; i < width; i++) {
		_band[off] = ~line[i];
		off += _height;
	}
	
	_line++;
	return 0;
}



/*
 * Exportation d'une bande
 * Export a band
 */

unsigned char* Band::exportBand(int algorithm, size_t *size)
{
	if (algorithm == 0)
		return _algorithm0(size);
	else if (algorithm == 0x11)
		return _algorithm11(size);
	else
		return NULL;
}

