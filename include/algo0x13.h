/*
 * 	    algo0x13.h                (C) 2007-2008, Aurélien Croc (AP²C)
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
#ifndef _ALGO0X13_H_
#define _ALGO0X13_H_

#ifndef DISABLE_JBIG

#include "algorithm.h"
extern "C" {
#   include "jbig85.h"
}

/**
  * @brief This class implements the compression algorithm 0x13.
  */
class Algo0x13 : public Algorithm
{
    protected:
        typedef struct bandList_s {
            BandPlane*          band;
            struct bandList_s*  next;
        } bandList_t;

        typedef struct info_s {
            bandList_t**        list;
            bandList_t*         last;
            unsigned char*      data;
            unsigned long       size;
            unsigned long       maxSize;
        } info_t;

    protected:
        bool                    _compressed;
        bandList_t*             _list;


    public:
        Algo0x13();
        virtual ~Algo0x13();

    public:
        static void             _callback(unsigned char *data, size_t len, void *arg);

    public:
        virtual BandPlane*      compress(const Request& request, 
                                    unsigned char *data, unsigned long width,
                                    unsigned long height);
};

#endif /* DISABLE_JBIG */

#endif /* _ALGO0X13_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

