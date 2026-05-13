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
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  $Id$
 * 
 */
#include <cstring>
#include <array>
#include <algorithm>
#include <cmath>
#include "sp_portable.h"
#include "page.h"
#include "document.h"
#include "band.h"
#include "errlog.h"
#include "request.h"


/*
 * Ouverture du fichier contenant la requête
 * Open the file which contains the job
 */
SP::Result<> Document::load([[maybe_unused]] const Request& request)
{
    _currentPage = 1;
    _lastPage = false;
    _raster.reset(cupsRasterOpen(SP_STDIN_FILENO, CUPS_RASTER_READ));
    if (!_raster) {
        ERRORMSG(_("Cannot open job"));
        return SP::Unexpected(SP::Error::RasterOpenError);
    }
    return {};
}

/*
 * Extraction d'une nouvelle page de la requête
 * Exact a new job page
 */
SP::Result<std::unique_ptr<Page>> Document::getNextRawPage([[maybe_unused]] const Request& request)
{
    cups_page_header2_t header;
    uint32_t pageWidth, pageWidthInB, pageHeight, clippingX = 0, clippingY = 0;
    uint32_t documentWidth, documentHeight, lineSize, planeSize, index = 0;
    uint32_t bytesToCopy, marginWidthInB = 0, marginHeight = 0;
    uint8_t colors;

    // Read the header
    if (_lastPage)
        return SP::Unexpected(SP::Error::EndOfJob);
    if (!_raster) {
        ERRORMSG(_("The raster hasn't been loaded"));
        return SP::Unexpected(SP::Error::RasterOpenError);
    }
    if (!cupsRasterReadHeader2(_raster.get(), &header) || !header.cupsBytesPerLine ||
        !header.PageSize[1]) {
        DEBUGMSG(_("No more pages"));
        _lastPage = true;
        return SP::Unexpected(SP::Error::EndOfJob);
    }

    // Input Sanitization: Validate header dimensions to prevent excessive resource allocation
    if (header.cupsWidth > 20000 || header.cupsHeight > 30000) {
        ERRORMSG(_("Invalid page dimensions in CUPS header: %ux%u"), 
                 header.cupsWidth, header.cupsHeight);
        return SP::Unexpected(SP::Error::InconsistentData);
    }

    // Make some calculations and store important data
    auto page = std::make_unique<Page>();
    page->setXResolution(header.HWResolution[0]);
    page->setYResolution(header.HWResolution[1]);
    colors = header.cupsColorSpace == CUPS_CSPACE_K ? 1 : 4;
    documentWidth = (header.cupsWidth + 7) & ~7;
    documentHeight = header.cupsHeight;
    lineSize = header.cupsBytesPerLine / colors;
    pageWidth = (static_cast<uint32_t>(std::ceil(page->convertToXResolution(request.
        printer()->pageWidth()))) + 7) & ~7;
    pageHeight = static_cast<uint32_t>(std::ceil(page->convertToYResolution(request.printer()->
        pageHeight())));
    
    // Safety check for calculated dimensions
    if (pageWidth == 0 || pageHeight == 0) {
        ERRORMSG(_("Resolved page dimensions are zero"));
        return SP::Unexpected(SP::Error::InconsistentData);
    }

    marginWidthInB = (static_cast<uint32_t>(std::ceil(page->convertToXResolution(header.Margins[0]))) + 7) / 8;
    marginHeight = static_cast<uint32_t>(std::ceil(page->convertToYResolution(header.Margins[1])));
    pageWidthInB = (pageWidth + 7) / 8;
    planeSize = pageWidthInB * pageHeight;

    // Last line components validation
    if (planeSize > 1024 * 1024 * 512) { // 512MB safety cap per plane
        ERRORMSG(_("Plane size is too large: %u"), planeSize);
        return SP::Unexpected(SP::Error::MemoryError);
    }

    page->setWidth(pageWidth);
    page->setHeight(pageHeight);
    page->setColorsNr(colors);
    page->setPageNr(_currentPage);
    page->setCompression(header.cupsCompression);
    page->setCopiesNr(header.NumCopies);

    // Calculate clippings and margins
    if (lineSize > pageWidthInB - 2 * marginWidthInB) {
        clippingX = (lineSize - (pageWidthInB - 2 * marginWidthInB)) / 2;
        bytesToCopy = pageWidthInB - 2 * marginWidthInB;
    } else {
        clippingX = 0;
        marginWidthInB = (pageWidthInB - lineSize) / 2;
        bytesToCopy = lineSize;
    }

    if (documentHeight > pageHeight - 2 * marginHeight) {
        clippingY = (documentHeight - (pageHeight - 2 * marginHeight)) / 2;
        index = pageWidthInB * marginHeight;
    } else {
        clippingX = 0; // redundant but safe
        index = pageWidthInB * ((pageHeight - documentHeight) / 2);
    }
    documentHeight -= clippingY;
    pageHeight -= 2 * marginHeight;
    clippingY *= colors;
    
    std::vector<uint8_t> line(header.cupsBytesPerLine);
    DEBUGMSG(_("Document width=%u height=%u"), documentWidth, documentHeight);
    DEBUGMSG(_("Page width=%u (%u) height=%u"), pageWidth, pageWidthInB, pageHeight);
    DEBUGMSG(_("Margin width in bytes=%u height=%u"), marginWidthInB, marginHeight);
    DEBUGMSG(_("Clipping X=%u Y=%u"), clippingX, clippingY);
    DEBUGMSG(_("Line size=%u, Plane size=%u, bytes to copy=%u"), lineSize, planeSize, bytesToCopy);

    // Prepare planes and clip vertically the document if needed
    std::array<std::vector<uint8_t>, 4> planes;
    for (uint8_t i = 0; i < colors; i++) {
        try {
            planes[i].resize(planeSize, 0);
        } catch (const std::bad_alloc&) {
            ERRORMSG(_("Failed to allocate plane buffer"));
            return SP::Unexpected(SP::Error::MemoryError);
        }
    }

    while (clippingY) {
        if (cupsRasterReadPixels(_raster.get(), line.data(), lineSize) < 1) {
            ERRORMSG(_("Cannot read pixel line during clipping"));
            return SP::Unexpected(SP::Error::RasterReadError);
        }
        clippingY--;
    }

    // Load the bitmap
    while (pageHeight && documentHeight) {
        for (uint8_t i = 0; i < colors; i++) {
            if (cupsRasterReadPixels(_raster.get(), line.data(), lineSize) < 1) {
                ERRORMSG(_("Cannot read pixel line during data load"));
                return SP::Unexpected(SP::Error::RasterReadError);
            }
            auto destSpan = std::span(planes[i]).subspan(index + marginWidthInB, bytesToCopy);
            auto srcSpan = std::span(line).subspan(clippingX, bytesToCopy);
            std::ranges::copy(srcSpan, destSpan.begin());
        }
        index += pageWidthInB;
        pageHeight--;
        documentHeight--;
    }

    // Finish to clip vertically the document
    documentHeight *= colors;
    while (documentHeight) {
        if (cupsRasterReadPixels(_raster.get(), line.data(), lineSize) < 1) {
            ERRORMSG(_("Cannot read pixel line during final clipping"));
            return SP::Unexpected(SP::Error::RasterReadError);
        }
        documentHeight--;
    }
    _currentPage++;

    for (uint8_t i = 0; i < colors; i++)
        page->setPlaneBuffer(i, std::move(planes[i]));

    DEBUGMSG(_("Page %u (%u×%u on %u×%u) has been successfully loaded into memory"), 
        page->pageNr(), header.cupsWidth, header.cupsHeight, 
        page->width(), page->height());

    return std::move(page);
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

