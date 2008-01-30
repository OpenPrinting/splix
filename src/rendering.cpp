/*
 * 	    rendering.cpp             (C) 2006-2008, Aurélien Croc (AP²C)
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
#include "rendering.h"
#include "qpdl.h"
#include "page.h"
#include "errlog.h"
#include "colors.h"
#include "request.h"
#include "printer.h"
#include "compress.h"
#include "document.h"

bool render(const Request& request)
{
    Document document;
    Page* page;

    // Load the document
    if (!document.load(request)) {
        ERRORMSG(_("Error while rendering the request. Check the previous "
            "message"));
        return false;
    }

    // Send the PJL Header
    request.printer()->sendPJLHeader(request);

    // Send each page
    page = document.getNextRawPage(request);
    while (page) {
        applyBlackOptimization(page);
        page->setCompression(0x11); // XXX XXX XXX FIXME TO REMOVE
        if (compressPage(request, page)) {
            if (!renderPage(request, page))
                ERRORMSG(_("Error while rendering the page. Check the previous "
                            "message. Trying to print the other pages."));
        } else
            ERRORMSG(_("Error while compressing the page. Check the previous "
                "message. Trying to print the other pages."));
        delete page;
        fprintf(stderr, "PAGE: %lu %lu\n", page->pageNr(), page->copiesNr());
        page = document.getNextRawPage(request);
    }

    // Send the PJL footer
    request.printer()->sendPJLFooter(request);

    return true;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

