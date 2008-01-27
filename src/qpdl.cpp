/*
 * 	    qpdl.cpp                  (C) 2006-2008, Aurélien Croc (AP²C)
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
#include "qpdl.h"
#include "page.h"
#include "band.h"
#include "errlog.h"
#include "request.h"
#include "bandplane.h"

bool renderPage(const Request& request, Page* page)
{
    unsigned char header[0x11];
    unsigned char duplex=0, tumble=0;

    if (!page) {
        ERRORMSG(_("Try to render a NULL page"));
        return false;
    }

    // Get the duplex values
    

    // Send the page header
    header[0x0] = 0;                                // Signature
    header[0x1] = page->yResolution() / 100;        // Y Resolution
    header[0x2] = page->copiesNr() >> 8;            // Number of copies 8-15
    header[0x3] = page->copiesNr();                 // Number of copies 0-7
    header[0x4] = request.printer()->paperType();   // Paper type
    header[0x5] = page->width() >> 8;               // Printable area width
    header[0x6] = page->width();                    // Printable area width
    header[0x7] = page->height() >> 8;              // Printable area height
    header[0x8] = page->height();                   // Printable area height
    header[0x9] = request.printer()->paperSource(); // Paper source
    header[0xa] = request.printer()->unknownByte1();// ??? XXX
    header[0xb] = duplex;                           // Duplex
    header[0xc] = tumble;                           // Tumble
    header[0xd] = request.printer()->unknownByte2();// ??? XXX
    header[0xe] = request.printer()->qpdlVersion(); // QPDL Version
    header[0xf] = request.printer()->unknownByte3();// ??? XXX
    header[0x10] = page->xResolution() / 100;       // X Resolution

    return false;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

