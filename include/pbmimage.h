/*
 * 	pbmimage.h		(C) 2006, Aurélien Croc (AP²C)
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
#ifndef PBMIMAGE_H_
#define PBMIMAGE_H_

#include <inttypes.h>
#include <stdio.h>
#include "printer.h"
#include "document.h"

class PbmImage : public Document
{
    protected:
        const char*             _blackFile;
        const char*             _cyanFile;
        const char*             _magentaFile;
        const char*             _yellowFile;
        FILE*                   _black;
        FILE*                   _cyan;
        FILE*                   _magenta;
        FILE*                   _yellow;

        uint32_t                _width;
        uint32_t                _height;
        uint32_t                _lineSize;
        uint32_t                _line;
        unsigned char*          _lineBuffer;

        bool                    _color;
        uint8_t                 _currentColor;

    public:
        PbmImage(const char *black, const char *cyan, 
            const char *magenta, const char *yellow);
        virtual ~PbmImage();

    public:
        virtual void            unload();
        virtual int             load();
        virtual int             loadPage(Printer *printer);
        virtual int             readLine();

    public:
        virtual unsigned long   width() const {return _width;}
        virtual unsigned long   height() const {return _height;}
        virtual unsigned long   lineSize() const {return _lineSize;}
        virtual unsigned char*  lineBuffer() const {return _lineBuffer;}
        virtual unsigned long   page() const {return 1;}

    public:
        virtual bool            isColor() const {return _color;}
};

#endif /* DOCUMENT_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

