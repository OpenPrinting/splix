/*
 * 	    algorithm.h               (C) 2006-2008, Aurélien Croc (AP²C)
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
#ifndef _ALGORITHM_H_
#define _ALGORITHM_H_

class Request;
class BandPlane;

/**
  * @brief This super class is an interface to implement a compression 
  *        algorithm.
  */
class Algorithm
{
    protected:

    public:
        /**
          * Initialize the instance.
          */
        Algorithm();
        /**
          * Destroy the instance.
          */
        virtual ~Algorithm();

    public:
        /**
          * Compress data.
          * @param request the request instance
          * @param data the data to compress
          * @param width the width of the data / band / page
          * @param height the height of the data / band / page
          * @return a pointer to a @ref BandPlane instance or NULL.
          */
        virtual BandPlane*      compress(const Request& request, 
                                    unsigned char *data, unsigned long width,
                                    unsigned long height) = 0;
        /**
          * Reverse line and column.
          * the byte at (x=1, y=0) is placed at (x=0, y=1) etc.
          * This is used by algorithm 0x11 (at least).
          * @return TRUE if this operation is needed. Otherwise returns FALSE.
          */
        virtual bool            reverseLineColumn() {return false;}
        /**
          * Inverse the byte.
          * Do a NOT operation on each byte.
          * @return TRUE if this operation is needed. Otherwise returns FALSE.
          */
        virtual bool            inverseByte() {return false;}
};

#endif /* _ALGORITHM_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

