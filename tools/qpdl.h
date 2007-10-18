/*
 *      qpdl.h                  (C) 2006-2007, Aurélien Croc (AP²C)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *   $Id$
 */
#ifndef _QPDL_H_
#define _QPDL_H_

#include <QtCore/qglobal.h>
#include "page.h"

class QFile;
class QTextStream;

class QPDLDocument
{
    public:
        enum Type {
            SPL2,
            SPLc,
        };

        enum Color {
            Cyan    = 1,
            Magenta = 2,
            Yellow  = 3,
            Black   = 4,
        };

    protected:
        quint8                  _qpdl;
        quint16                 _width;
        quint32                 _pageNr;
        quint8                  _duplex;
        quint8                  _tumble;
        quint16                 _height;
        quint16                 _nrCopies;
        quint8                  _paperType;
        quint16                 _resolutionX;
        quint16                 _resolutionY;
        quint8                  _paperSource;
        quint8                  _currentBandNr;
        char                    _unknown[3];

        bool                    _quiet;
        bool                    _decompression;
        bool                    _dump;
        Type                    _type;
        Page                    _page;

    protected:
        enum Result {
            Error   = -1,
            Ok      = 0,
            End     = 1,
        };

    protected:
        bool                    _processBandAnalysis(quint8 color, 
                                    quint16 width, quint16 height, 
                                    quint8 compression, QByteArray& content,
                                    QTextStream& out, QTextStream& err);
        Result                  _readPageHeader(QFile& data, QTextStream& out,
                                    QTextStream& err);
        Result                  _readPageContent(QFile& data, QTextStream& out,
                                    QTextStream& err);

    public:
        QPDLDocument();

    public:
        void                    setQuiet(bool quiet) {_quiet = quiet;}
        void                    setDecompressionState(bool state)
                                {_decompression = state;}
        void                    setDump(bool state) {_dump = state;}
        void                    setType(Type type) {_type = type;}

        bool                    decompressionState() const
                                {return _decompression;}
        Type                    type() const {return _type;}
        quint32                 page() const {return _pageNr;}
        quint32                 currentBandNr() const {return _currentBandNr;}

    public:
        bool                    parse(QFile& data, QTextStream& out, 
                                    QTextStream& err);
};

#endif /* _QPDL_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8 : */

