/*
 *      page.h                  (C) 2006-2007, Aurélien Croc (AP²C)
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
#ifndef _PAGE_H_
#define _PAGE_H_

#include <QtCore/QFile>

class QPDLDocument;
class QByteArray;
class QTextStream;

class Page
{
    protected:
        bool                    _be;
        quint8                  _version;

        QPDLDocument*           _qpdl;
        QFile                   _files[4];
        QByteArray              _layers[4];
        quint8                  _lastBand[4];
        quint16                 _width[4];
        quint16                 _height[4];
        quint8                  _maxColors;

    protected:
        void                    _closeFiles();
        bool                    _openFiles(quint8 compression,
                                    const QString& extension, QTextStream& err);

        bool                    _dump(quint8 color, quint16 width, 
                                    quint16 height, QByteArray& data,
                                    QTextStream& err);
        bool                    _compression0x11(quint8 color, quint16 width, 
                                    quint16 height, QByteArray& data,
                                    QTextStream& err);
        bool                    _compression0x13(quint8 color, quint16 width, 
                                    quint16 height, QByteArray& data,
                                    QTextStream& err);

    public:
        Page();

    public:
        void                    setQPDLDocument(QPDLDocument* qpdl) 
                                    {_qpdl = qpdl;}
        void                    setSubHeaderVersion(quint8 version)
                                    {_version = version;}
        void                    setBE(bool be) {_be = be;}
        void                    clear();

    public:
        bool                    process(quint8 color, quint16 width, 
                                    quint16 height, quint8 compression,
                                    QByteArray& content, QTextStream& out, 
                                    QTextStream& err);
        void                    flush();

        quint32                 read32(const QByteArray& data, quint32 i);
        quint16                 read16(const QByteArray& data, quint32 i);
        quint8                  read8(const QByteArray& data, quint32 i);
};

#endif /* _PAGE_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8 : */

