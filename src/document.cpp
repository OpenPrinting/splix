/*
 * 	    document.cpp              (C) 2006-2008, Aurélien Croc (AP²C)
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
#include <math.h>
#include "page.h"
#include "errlog.h"

/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
Document::Document()
{
    _raster = NULL;
    _currentPage = 0;
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
    _currentPage = 0;
    _lastPage = false;
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
Page* Document::getNextRawPage(const Request& request)
{
    cups_page_header_t header;
    unsigned long pageWidth, pageWidthInB, pageHeight, clippingX=0, clippingY=0;
    unsigned long documentWidth, documentHeight, lineSize, planeSize, index=0;
    unsigned long bytesToCopy, marginWidthInB=0, marginHeight=0;
    unsigned char *line, *planes[4];
    unsigned char colors;
    Page *page;

    // Read the header
    if (_lastPage)
        return NULL;
    if (!_raster) {
        ERRORMSG(_("The raster hasn't been loaded"));
        return NULL;
    }
    if (!cupsRasterReadHeader(_raster, &header) || !header.cupsBytesPerLine ||
        !header.PageSize[1]) {
        DEBUGMSG(_("No more pages"));
        _lastPage = true;
        return NULL;
    }

    // Make some calculations and store important data
    page = new Page;
    page->setXResolution(header.HWResolution[0]);
    page->setYResolution(header.HWResolution[1]);
    colors = header.cupsColorSpace == CUPS_CSPACE_K ? 1 : 4;
    documentWidth = (header.cupsWidth + 7) & ~7;
    documentHeight = header.cupsHeight;
    lineSize = header.cupsBytesPerLine / colors;
    pageWidth = ceill(page->convertToXResolution(header.PageSize[0]));
    pageHeight = ceill(page->convertToYResolution(header.PageSize[1]));
    marginWidthInB =(ceill(page->convertToXResolution(header.Margins[0]))+7)/8; 
    marginHeight = ceill(page->convertToYResolution(header.Margins[1]));
    pageWidth = (pageWidth + 7) & ~7;
    pageWidthInB = (pageWidth + 7) / 8;
    planeSize = pageWidthInB * pageHeight;
    page->setWidth(pageWidth);
    page->setHeight(pageHeight);
    page->setColorsNr(colors);
    page->setPageNr(_currentPage);
    page->setCompression(header.cupsCompression);
    page->setCopiesNr(header.NumCopies);

    // Calculate clippings and margins
    if (lineSize > pageWidthInB) {
        clippingX = (lineSize - pageWidthInB) / 2;
        bytesToCopy = pageWidthInB;
    } else {
        if (lineSize + marginWidthInB > pageWidthInB)
            marginWidthInB = pageWidthInB - lineSize;
        bytesToCopy = lineSize;
    }
    if (header.cupsHeight > pageHeight)
        clippingY = (header.cupsHeight - pageHeight) / 2;
    else {
        if (documentHeight + marginHeight > pageHeight)
            index = pageWidthInB * (pageHeight - documentHeight);
        else
            index = pageWidthInB * marginHeight;
    }
    documentHeight -= clippingY;
    clippingY *= colors;
    line = new unsigned char[header.cupsBytesPerLine];


    // Prepare planes and clip vertically the document if needed
    for (unsigned char i=0; i < colors; i++) {
        planes[i] = new unsigned char[planeSize];
        memset(planes[i], 0, planeSize);
    }
    while (clippingY) {
        if (cupsRasterReadPixels(_raster, line, lineSize) < 1) {
            ERRORMSG(_("Cannot read pixel line"));
            for (unsigned int i=0; i < colors; i++)
                delete[] planes[i];
            delete[] line;
            delete page;
            return NULL;
        }
        clippingY--;
    }

    // Load the bitmap
    while (pageHeight && documentHeight) {
        for (unsigned int i=0; i < colors; i++) {
            if (cupsRasterReadPixels(_raster, line, lineSize) < 1) {
                ERRORMSG(_("Cannot read pixel line"));
                for (unsigned int j=0; j < colors; j++)
                    delete[] planes[j];
                delete[] line;
                delete page;
                return NULL;
            }
            memcpy(planes[i] + index + marginWidthInB, line + clippingX, 
                bytesToCopy);
        }
        index += pageWidthInB;
        pageHeight--;
        documentHeight--;
    }

    // Finish to clip vertically the document
    documentHeight *= colors;
    while (documentHeight) {
        if (cupsRasterReadPixels(_raster, line, lineSize) < 1) {
            ERRORMSG(_("Cannot read pixel line"));
            for (unsigned int j=0; j < colors; j++)
                delete[] planes[j];
            delete[] line;
            delete page;
            return NULL;
        }
        documentHeight--;
    }
    _currentPage++;

    for (unsigned int i=0; i < colors; i++)
        page->setPlaneBuffer(i, planes[i]);

    DEBUGMSG(_("Page %lu (%u×%u on %lu×%lu) has been successfully loaded into "
        "memory"), page->pageNr(), header.cupsWidth, header.cupsHeight, 
        page->width(), page->height());
/** @todo to remove */
// TO REMOVE XXX XXX XXX
#if 0
    for (unsigned int i=0; i < colors; i++) {
        FILE *prout;
        const char *fn;
        if (i == 0) fn = "/home/aurelien/test1.pbm";
        else if (i == 1) fn = "/home/aurelien/test2.pbm";
        else if (i == 2) fn = "/home/aurelien/test3.pbm";
        else if (i == 3) fn = "/home/aurelien/test4.pbm";
        prout = fopen(fn, "w");
        fprintf(prout, "P4\n%lu %lu\n\n", page->width(), page->height());
        fwrite(planes[0], 1, pageWidthInB * page->height(), prout);
        fclose(prout);
    }
#endif

    delete[] line;
    return page;
}



/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

