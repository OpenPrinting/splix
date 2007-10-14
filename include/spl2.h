/*
 * 	spl2.h			(C) 2006, Aurélien Croc (AP²C)
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
#ifndef SPL2_H_
#define SPL2_H_

#include <stdio.h>

class Document;
class Printer;
class Task;
class Band;

class SPL2
{
    protected:
        FILE*                   _output;
        Printer*                _printer;

    protected:
        int                     _writeColorBand(Band *band, int color);

    public:
        SPL2();
        virtual ~SPL2();

    public:
        void                    setOutput(FILE* output) {_output = output;}
        void                    setPrinter(Printer *printer) {_printer=printer;}

    public:
        int                     beginDocument();
        int                     closeDocument();

        int                     printPage(Document *document, 
                                    unsigned long nrCopies);
};

#endif /* SPL2_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

