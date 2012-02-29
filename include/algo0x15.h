/*
 * 	    algo0x15.h                (C) 2007-2008, Aurélien Croc (AP²C)
 *                                    This file is a SpliX derivative work
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
 *  $Id: algo0x15.h 287 2011-02-19 19:10:22Z tillkamppeter $
 *  --
 *  This code was written by Leonardo Hamada
 * 
 */
#ifndef _ALGO0X15_H_
#define _ALGO0X15_H_

#ifndef DISABLE_JBIG

#include <stddef.h>
#include "algorithm.h"
/**
  * @brief This class implements the compression algorithm 0x15.
  */
class Algo0x15 : public Algorithm
{
    protected:
        bool                    _error;
        bool                    _has_bih;
        unsigned char           _bih[20];
        unsigned char*          _data;
        unsigned long           _size;
        unsigned long           _maxSize;

    public:
        Algo0x15();
        virtual ~Algo0x15();

    public:
        static void             _callback(unsigned char *data, size_t len, void *arg);

    public:
        virtual BandPlane*      compress(const Request& request, 
                                    unsigned char *data, unsigned long width,
                                    unsigned long height);
        /* Returns BIH for the compressed image band,
           after compress has been called. */
        const unsigned char*    getBIHdata() const { return _bih; } 
};

#endif /* DISABLE_JBIG */

#endif /* _ALGO0X15_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

