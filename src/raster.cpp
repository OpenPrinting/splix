/*
 * 	raster.cpp		(C) 2006, Aurélien Croc (AP²C)
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
#include "raster.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
//#include <cups/ppd.h>
#include "error.h"

/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
Raster::Raster(const char *job, const char *user, const char *title, 
	const char *copies, const char *options, const char *file)
{
	_jobId = job;
	_user = user;
	_title = title;
	_copies = copies;
	_options = options;
	_file = file;

	_width = 0;
	_height = 0;
	_lineSize = 0;
	_line = 0;
	_page = 0;
	_lineBuffer = NULL;
}

Raster::~Raster()
{
	unload();
}



/*
 * Chargement de l'image
 * Load the image
 */
void Raster::unload()
{
	if (_lineBuffer)
		delete[] _lineBuffer;
	cupsRasterClose(_ras);
}

int Raster::load()
{
	// Open the raster file if needed
	if (_file && (_fd = open(_file, O_RDONLY)) == -1) {
		fprintf(stderr, _("ERROR: Unable to open the raster file %s\n"),
			_file);
		sleep(1);
		return -1;
	}

	_ras = cupsRasterOpen(_fd, CUPS_RASTER_READ);
	return 0;
}

int Raster::loadPage(Printer *printer)
{
	if (!cupsRasterReadHeader(_ras, &_header)) {
		DEBUG("Plus de page");
		return 1;
	}
	_width = _header.cupsWidth;
	_height = _header.cupsHeight;
	_totalLines = _height;
	_lineSize = _header.cupsBytesPerLine;
	_line = 0;
	_page++;

	// Configure the printer 
	printer->setResolution(_header.HWResolution[0],_header.HWResolution[1]);
	printer->setPageSizeX(_header.PageSize[0]);
	printer->setPageSizeY(_header.PageSize[1]);
	printer->setMarginX(_header.ImagingBoundingBox[0]);
	printer->setMarginY(_header.ImagingBoundingBox[1]);
	printer->setAreaX(_header.PageSize[0] - _header.ImagingBoundingBox[0]);
	printer->setAreaY(_header.PageSize[1] - _header.ImagingBoundingBox[1]);
	printer->setPrintableX(_header.ImagingBoundingBox[2] - 
		_header.ImagingBoundingBox[0]);
	printer->setPrintableY(_header.ImagingBoundingBox[3] - 
		_header.ImagingBoundingBox[1]);

	// Get some document informations
	_color = _header.cupsColorSpace == CUPS_CSPACE_K ? false : true;
	printer->setCompVersion(_header.cupsCompression);

	if (_color) {
		_totalLines = _totalLines * 4;
		_lineSize = _lineSize >> 2;
	}

	return 0;
}



/* 
 * Lecture d'une ligne
 * Read a line
 */
int Raster::readLine()
{
	if (!_ras)
		return -1;
	if (!_lineBuffer)
		_lineBuffer = new unsigned char[_lineSize];

	/*
	 * so that we can round up to bandHeight, we return an empty line
	 * after reading more than _height lines.
	 * -- Keith White
	 */
	if (_line >= _totalLines) {
		memset(_lineBuffer, 0x00, _lineSize);
		return _lineSize;
	}

	if (cupsRasterReadPixels(_ras, _lineBuffer, _lineSize) < 1) {
		ERROR(_("Raster::readLine: Cannot read image data"));
		return -1;
	}
	_line++;
	return _lineSize;
}

