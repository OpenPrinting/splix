/*
 * 	    request.h                 (C) 2006-2007, Aurélien Croc (AP²C)
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
#ifndef _REQUEST_H_
#define _REQUEST_H_

#include <cups/ppd.h>
#include "printer.h"

class Request 
{
    public:
        enum Duplex {
            None,
            LongEdge,
            ShortEdge,
            Manual,
        };

    protected:
        ppd_file_t*             _ppd;

        const char*             _username;
        const char*             _jobname;
        const char*             _jobtitle;
        unsigned long           _copiesNr;

        Printer                 _printer;

        Duplex                  _duplex;

    public:
        Request();
        virtual ~Request();

    public:
        /**
          * Load a new request.
          * @param ppd the PPD file handle
          * @param jobname the job ID
          * @param username the name of the user which make this job
          * @param jobtitle the job title
          * @param copiesNr the number of copies to print
          * @return TRUE if the request has been successfully loaded. Otherwise
          *         it returns FALSE.
          */
        bool                    loadRequest(ppd_file_t* ppd, 
                                    const char *jobname, const char *username, 
                                    const char *jobtitle, 
                                    unsigned long copiesNr);

    public:
        /**
          * @return the PPD file handle.
          */
        ppd_file_t*             ppd() const {return _ppd;}
        /**
          * @return the printer instance.
          */
        const Printer*          printer() const {return &_printer;}

};

#endif /* _REQUEST_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

