/*
 * 	    algo0x11.h                (C) 2006-2008, Aurélien Croc (AP²C)
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
#ifndef _ALGO0X11_H_
#define _ALGO0X11_H_

#include "algorithm.h"
#include <inttypes.h>

#define COMPRESS_SAMPLE_RATE    0x800
#define TABLE_PTR_SIZE          0x40
#define MAX_UNCOMPRESSED_BYTES  0x80
#define MAX_COMPRESSED_BYTES    0x202
#define MIN_COMPRESSED_BYTES    0x2

#define COMPRESSION_FLAG        0x80

/**
  * @brief This class implements the compression algorithm 0x11.
  */
class Algo0x11 : public Algorithm
{
    protected:
        uint32_t                _ptrArray[TABLE_PTR_SIZE];

    protected:
        static int              __compare(const void *n1, const void *n2);
        bool                    _lookupBestOccurs(const unsigned char* data,
                                    unsigned long size);
        bool                    _compress(const unsigned char *data, 
                                    unsigned long size, 
                                    unsigned char* &output, 
                                    unsigned long &outputSize);

    public:
        Algo0x11();
        virtual ~Algo0x11();

    public:
        virtual BandPlane*      compress(const Request& request, 
                                    unsigned char *data, unsigned long width,
                                    unsigned long height);
        virtual bool            reverseLineColumn() {return true;}
        virtual bool            inverseByte() {return true;}
        virtual bool            splitIntoBands() {return true;}
};

#endif /* _ALGO0X11_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

