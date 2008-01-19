/*
 * 	    document.h                (C) 2006-2007, Aurélien Croc (AP²C)
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
#ifndef _DOCUMENT_H_
#define _DOCUMENT_H_

#include <cups/raster.h>

class Page;
class Request;

/**
  * @brief This class converts the job into usable pages.
  *
  * Two main methods permits to get a bitmap page (used by the compression
  * algorithms) and to get the compressed pages (used by the QPDL render).
  */
class Document
{
    protected:
        cups_raster_t*          _raster;

    public:
        /**
          * Initialize the instance.
          */
        Document();
        /**
          * Destroy the instance.
          */
        virtual ~Document();

    public:
        /**
          * Load the file which contains the job.
          * The file have to be opened on the file descriptor 0 (STDIN_FILENO)
          * and be formatted as CUPS Raster.
          * @return TRUE if it has been successfully opened. Otherwise it
          *         returns FALSE.
          */
        bool                    load();

        Page                    getNextRawPage(const Request& request);
};

#endif /* _DOCUMENT_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

