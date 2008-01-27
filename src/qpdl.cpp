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
#include <errno.h>
#include <unistd.h>
#include "page.h"
#include "band.h"
#include "errlog.h"
#include "request.h"
#include "bandplane.h"

bool renderPage(const Request& request, Page* page)
{
    unsigned char duplex=0, tumble=0, paperSource;
    unsigned char header[0x11];

    if (!page) {
        ERRORMSG(_("Try to render a NULL page"));
        return false;
    }

    // Get the duplex values
    paperSource = request.printer()->paperSource();
    switch (request.duplex()) {
        case Request::Simplex:
            duplex = 1;
            tumble = 0;
            break;
        case Request::LongEdge:
            duplex = 1;
            tumble = page->pageNr() % 2;
            break;
        case Request::ShortEdge:
            duplex = 0;
            tumble = page->pageNr() % 2;
            break;
        case Request::ManualLongEdge:
            duplex = 0;
            tumble = page->pageNr() % 2;
            if (tumble)
                paperSource = 3; // Multi source
            break;
        case Request::ManualShortEdge:
            /** @todo what about the Short edge? The page isn't rotated?  */
            break;
    }

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
    header[0x9] = paperSource;                      // Paper source
    header[0xa] = request.printer()->unknownByte1();// ??? XXX
    header[0xb] = duplex;                           // Duplex
    header[0xc] = tumble;                           // Tumble
    header[0xd] = request.printer()->unknownByte2();// ??? XXX
    header[0xe] = request.printer()->qpdlVersion(); // QPDL Version
    header[0xf] = request.printer()->unknownByte3();// ??? XXX
    header[0x10] = page->xResolution() / 100;       // X Resolution
    if (write(STDOUT_FILENO, (unsigned char*)&header, 0x11) == -1) {
        ERRORMSG(_("Error while sending data to the printer (%u)"), errno);
        return false;
    }

    // Send the page bands
    // TODO TODO TODO XXX XXX XXX
    /** @todo add the page band sending system */

    // Send the page footer
    header[0x0] = 1;                                // Signature
    header[0x1] = page->copiesNr() >> 8;            // Number of copies 8-15
    header[0x2] = page->copiesNr();                 // Number of copies 0-7
    if (write(STDOUT_FILENO, (unsigned char*)&header, 0x3) == -1) {
        ERRORMSG(_("Error while sending data to the printer (%u)"), errno);
        return false;
    }

    return true;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

