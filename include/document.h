/*
 * 	    document.h                (C) 2006-2008, Aurélien Croc (AP²C)
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
#ifndef _DOCUMENT_H_
#define _DOCUMENT_H_

#include <memory>
#include <cups/raster.h>
#include "sp_result.h"

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
    private:
        struct RasterDeleter {
            void operator()(cups_raster_t* r) const {
                if (r) cupsRasterClose(r);
            }
        };

    protected:
        std::unique_ptr<cups_raster_t, RasterDeleter> _raster;
        uint32_t                                      _currentPage = 1;
        bool                                          _lastPage = false;

    public:
        /**
          * Initialize the instance.
          */
        Document() = default;
        /**
          * Destroy the instance.
          */
        virtual ~Document() = default;

    public:
        /**
          * Load the file which contains the job.
          * The file have to be opened on the file descriptor 0 (STDIN_FILENO)
          * and be formatted as CUPS Raster.
          * @param request the request instance
          * @return a Result indicating success or error.
          */
        SP::Result<>            load(const Request& request);

        /**
          * Load the next page into memory and store all the needed information
          * in a @ref Page instance.
          * @param request the request instance
          * @return a @ref Page instance containing the current page or a specific error.
          */
        SP::Result<std::unique_ptr<Page>> getNextRawPage(const Request& request);
        /**
          * @return the number of pages or 0 if its number is not yet known.
          */
        uint32_t                numberOfPages() const 
                                    {return _lastPage ? _currentPage - 1: 0;}
};

#endif /* _DOCUMENT_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */
