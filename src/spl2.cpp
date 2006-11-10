/*
 * 	spl2.cpp		(C) 2006, Aurélien Croc (AP²C)
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
 * ---
 *  Thanks to Keith White for  modifications, corrections and adds.
 * 
 */
#include "spl2.h"
#include "printer.h"
#include "document.h"
#include "error.h"
#include "band.h"
#include <stdint.h>


/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
SPL2::SPL2()
{
	_printer = NULL;
	_output = NULL;
}

SPL2::~SPL2()
{
}


/* 
 * Génération de l'en-tête et du pied de page PJL
 * Write PJL header and footer
 */
int SPL2::beginDocument()
{
	if (!_output || !_printer) {
		ERROR(_("SPL2::beginDocument: called with NULL parameters"));
		return -1;
	}

	_printer->newJob(_output);
	fprintf(_output, "@PJL ENTER LANGUAGE = QPDL\n");
	DEBUG("Envoie de l'en-tête du document JPL");
	return 0;
}

int SPL2::closeDocument()
{

	if (!_output || !_printer) {
		ERROR(_("SPL2::closeDocument: called with NULL parameters"));
		return -1;
	}
	_printer->endJob(_output);
	DEBUG("Envoie du pied de page du document JPL");
	return 0;
}


/*
 * Impression d'une page
 * Impress a page
 */
int SPL2::printPage(Document *document, unsigned long nrCopies)
{
	unsigned long width, height, clippingX, clippingY;
	unsigned long bandNumber;
	unsigned long i;
	char header[0x11];
	char errors=0;
	Band *band;

	if (!document) {
		ERROR(_("SPL2::printPage: called with NULL parameter"));
		return -2;
	}

	// Load a new page
	if (document->loadPage(_printer))
		return -1;

	// Send page header FIXME
	header[0x0] = 0;				// Signature
	header[0x1] = _printer->resolutionY() / 100;	// Y Resolution
	header[0x2] = nrCopies >> 8;			// Number of copies 8-15
	header[0x3] = nrCopies;				// Number of copies 0-7
	header[0x4] = _printer->paperType();		// Paper type
	header[0x5] = 0;				// Paper size if Custom
	header[0x6] = 0;				// Paper size if Custom
	header[0x7] = 0;				// Paper size if Custom
	header[0x8] = 0;				// Paper size if Custom
	header[0x9] = _printer->paperSource();		// Paper source
	header[0xa] = 0;				// ??? XXX
	header[0xb] = _printer->duplex() >> 8;		// Duplex
	header[0xc] = _printer->duplex();		// Duplex

	header[0xd] = _printer->docHeaderValues(2);
	header[0xe] = _printer->docHeaderValues(3);	// 0 = checksum absent?
	header[0xf] = _printer->docHeaderValues(4);
	if (_printer->resolutionY() != _printer->resolutionX())
		header[0x10] = _printer->resolutionX() / 100;
	else
		header[0x10] = 0;
	fwrite((char *)&header, 1, sizeof(header), _output);

	// Get the width, height, clipping X and clipping Y values
	if (document->width() <= _printer->areaX()) {
		clippingX = 0;
		width = document->width();
	} else if (document->width() < _printer->pageSizeX()) {
		clippingX = (unsigned long)(document->width() - 
			_printer->areaX());
		width = document->width();
	} else {
		clippingX = (unsigned long)_printer->marginX();
		width = (unsigned long) _printer->pageSizeX();
	}
	if (document->height() <= _printer->areaY()) {
		clippingY = 0;
		height = document->height();
	} else if (document->height() < _printer->pageSizeY()) {
		clippingY = (unsigned long)(document->height() - 
			_printer->areaY());
		height = document->height();
	} else {
		clippingY = (unsigned long)_printer->marginY();
		height = (unsigned long) _printer->pageSizeY();
	}

	// Create the band instance
	band = new Band((unsigned long)_printer->pageSizeX(), 
		(unsigned long)_printer->bandHeight());
	bandNumber = 0;
	band->setClipping(clippingX);

	// Clip vertically the document
	for (;clippingY; clippingY--)
		document->readLine();
	
	// Round up height to a multiple of bandHeight
	height += _printer->bandHeight() - (height % _printer->bandHeight());

	// Read and create each band
	for (i=0; i < height; i++) {
		int res;

		res = document->readLine();
		if (res < 0) {
			errors = 1;
			break;
		} else if (!res)
			break;
		if (band->addLine(document->lineBuffer(), 
			(res > width ? width : res))) {
			errors = 1;
			break;
		}

		// Compress and send the band if it's complete
		if (band->isFull()) {
			uint32_t checksum = 0;
			unsigned char *data;
			size_t size;

			// Compress
			if (!(data = band->exportBand(_printer->compVersion(), 
				&size))) {
				errors = 1;
				break;
			}

			// Do the checksum
			for (unsigned int j=0; j < size; j++)
				checksum += data[j];

			// Write the header
			header[0x0] = 0xC;			// Signature
			header[0x1] = bandNumber;		// Band number
			header[0x2] = band->width() >> 8;	// Band width
			header[0x3] = band->width();		// Band width
			header[0x4] = band->height() >> 8;	// Band height
			header[0x5] = band->height();		// Band height
			header[0x6] = _printer->compVersion();	// Comp version
			header[0x7] = (size + 4) >> 24;		// data length
			header[0x8] = (size + 4) >> 16;		// data length
			header[0x9] = (size + 4) >> 8;		// data length
			header[0xa] = (size + 4);		// data length
			fwrite((char *)&header, 1, 0xb, _output);

			// Write the data
			fwrite(data, 1, size, _output);
			delete[] data;

			// Write the checksum
			header[0x0] = checksum >> 24;
			header[0x1] = checksum >> 16;
			header[0x2] = checksum >> 8;
			header[0x3] = checksum;
			fwrite((char *)&header, 1, 0x4, _output);

			band->clean();
			bandNumber++;
		}
	}
	delete band;
	if (errors)
		return -11;

	// Write the end of the page
	header[0x0] = 1;		// Signature
	header[0x1] = nrCopies >> 8;	// Number of copies 8-15
	header[0x2] = nrCopies;		// Number of copies 0-7
	fwrite((char *)&header, 1, 3, _output);
	
	return 0;
}


