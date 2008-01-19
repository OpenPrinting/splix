/*
 * 	    page.h                    (C) 2006-2007, Aurélien Croc (AP²C)
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
#ifndef _PAGE_H_
#define _PAGE_H_


/**
  * @brief This class contains a page representation.
  *
  * There are two steps. During the first step, the instance contains the bitmap
  * representation of the page. During the second step, the bitmap
  * representation is freed and is replaced by bands representations.
  * Each band is linked to this instance and will be used by the QPDL render.
  *
  * When the page will be compressed this instance have to be sent to the cache
  * manager for waiting its use by the render code.
  */
class Page
{
    protected:

    public:
        /**
          * Initialize the page instance.
          */
        Page();
        /**
          * Destroy the instance
          */
        virtual ~Page();

    public:
};
#endif /* _PAGE_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

