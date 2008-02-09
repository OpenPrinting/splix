/*
 * 	    qpdl.h                    (C) 2006-2008, Aurélien Croc (AP²C)
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
#ifndef _QPDL_H_
#define _QPDL_H_

class Request;
class Page;

/**
  * Render a page and send the result to STDOUT.
  * If the output needs to be redirected to a file, please use the 
  * freopen function to redirect the output to a specific place.
  * @param request the request instance
  * @param page the page instance
  * @param lastPage set to TRUE if it's the last page (only used with manual
  *                 duplex)
  * @return TRUE if the page has been rendered into QPDL. Otherwise it returns
  *         FALSE.
  */
extern bool renderPage(const Request& request, Page* page, bool lastPage=false);
#endif /* _QPDL_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

