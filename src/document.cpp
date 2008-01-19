/*
 * 	    document.cpp              (C) 2006-2007, Aurélien Croc (AP²C)
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
#include "document.h"
#include <fcntl.h>
#include "page.h"
#include "errlog.h"

/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
Document::Document()
{
    _raster = NULL;
}

Document::~Document()
{
    if (_raster)
         cupsRasterClose(_raster);
}



/*
 * Ouverture du fichier contenant la requête
 * Open the file which contains the job
 */
bool Document::load()
{
    _raster = cupsRasterOpen(STDIN_FILENO, CUPS_RASTER_READ);
    if (!_raster) {
        ERRORMSG(_("Cannot open job"));
        return false;
    }
    return true;
}



/*
 * Extraction d'une nouvelle page de la requête
 * Exact a new job page
 */
Page Document::getNextRawPage(const Request& request)
{
    cups_page_header_t header;
    unsigned long printableWidth, printableHeight, clippingX, clippingY;
    unsigned long lineSize;
    unsigned char colors;
    unsigned char* line;
    Page page;

    // Read the header
    if (!_raster) {
        ERRORMSG(_("The raster hasn't been loaded"));
        return page;
    }
    if (!cupsRasterReadHeader(_raster, &header) || !header.cupsBytesPerLine) {
        DEBUGMSG(_("No more pages"));
        return page;
    }

    // Make some calculations
    lineSize = header.cupsBytesPerLine;
    printableWidth = header.ImagingBoundingBox[2] - 
        header.ImagingBoundingBox[0];
    printableHeight = header.ImagingBoundingBox[3] - 
        header.ImagingBoundingBox[1];
    line = new unsigned char[header.cupsBytesPerLine];


    // Load bitmaps
    colors = header.cupsColorSpace == CUPS_CSPACE_K ? 1 : 4;
    for (unsigned char i=0; i < colors; i++) {
    }

    DEBUGMSG("Image %u %u %u %u", header.ImagingBoundingBox[0], 
        header.ImagingBoundingBox[1], header.ImagingBoundingBox[2],
        header.ImagingBoundingBox[3]);
    DEBUGMSG("Margins %u %u", header.Margins[0], header.Margins[1]);
    DEBUGMSG("Copies %u", header.NumCopies);
    DEBUGMSG("Compression %u", header.cupsCompression);
    DEBUGMSG("Bits per color %u", header.cupsBitsPerColor);
    DEBUGMSG("Bytes per line %u", header.cupsBytesPerLine);
    DEBUGMSG("Height %u", header.cupsHeight);
    DEBUGMSG("Width %u", header.cupsWidth);
    DEBUGMSG("cupsColorSpace %u", header.cupsColorSpace);
    DEBUGMSG("pageSize %u %u", header.PageSize[0], header.PageSize[1]);
    DEBUGMSG("resolution %u %u", header.HWResolution[0], 
        header.HWResolution[1]);

    return Page();
}



/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

