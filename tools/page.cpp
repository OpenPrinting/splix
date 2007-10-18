/*
 *      algos.cpp               (C) 2006-2007, Aurélien Croc (AP²C)
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
#include "page.h"
#include <QtCore/QByteArray>
#include <QtCore/QTextStream>
#include "algo0x11.h"
#include "qpdl.h"
#include "i18n.h"

static const char* _colorsName[] = {"cyan","magenta","yellow","black"};


/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
Page::Page()
{
    _maxColors = 0;
    for (unsigned int i=0; i < 4; i++) {
        _layers[i].clear();
        _lastBand[i] = 0;
    }
    _qpdl = NULL;
}



/*
 * Gestion des fichiers
 * Files management
 */
void Page::_closeFiles()
{
    for (unsigned int i=0; i < _maxColors; i++)
        _files[i].close();
}

bool Page::_openFiles(quint8 compression, const QString& extension, 
    QTextStream& err)
{
    // Check if files are still opened
    if (_files[0].isOpen())
        return true;

    // Open it
    _maxColors =  _qpdl->type() == QPDLDocument::SPLc ? 4 : 1;
    for (unsigned int i=0; i < _maxColors; i++) {
        _files[i].setFileName(QString("page%1-%2.%3").arg(_qpdl->page(), 3, 
            10, QChar('0')).arg(_colorsName[i]).arg(extension));
        if (!_files[i].open(QIODevice::WriteOnly)) {
            err << QString(_("Page: cannot open file %1 in write mode")).
                arg(_files[i].fileName()) << endl;
            _closeFiles();
            return false;
        }
    }
    return true;
}



/*
 * Nettoyage
 * Clear all
 */
void Page::clear()
{
    _closeFiles();
    _maxColors = 0;
    for (unsigned int i=0; i < 4; i++) {
        _layers[i].clear();
        _lastBand[i] = 0;
        _width[i] = 0;
        _height[i] = 0;
    }
}



/*
 * Extraction des informations
 * Extract information
 */
quint32 Page::read32(const QByteArray& data, quint32 i)
{
    quint32 tmp;

    if (_be)
        tmp = ((quint8)data.at(i+0) << 24) + ((quint8)data.at(i+1) << 16) +
            ((quint8)data.at(i+2) << 8) + (quint8)data.at(i+3);
    else
        tmp = ((quint8)data.at(i+3) << 24) + ((quint8)data.at(i+2) << 16) +
            ((quint8)data.at(i+1) << 8) + (quint8)data.at(i+0);
    return tmp;
}

quint16 Page::read16(const QByteArray& data, quint32 i)
{
    quint16 tmp;

    if (_be)
        tmp = ((quint8)data.at(i+0) << 8) + (quint8)data.at(i+1);
    else
        tmp = ((quint8)data.at(i+1) << 8) + (quint8)data.at(i+0);
    return tmp;
}

quint8 Page::read8(const QByteArray& data, quint32 i)
{
    return (quint8)data.at(i+0);
}



/* 
 * Réception d'une bande pour traitement
 * Manage band data
 */
bool Page::process(quint8 color, quint16 width, quint16 height, 
    quint8 compression, QByteArray& content, QTextStream& out, 
    QTextStream& err)
{
    QString extension;
    quint32 size, state;

    // Check if the compression is supported
    if (compression == 0) 
        extension = "dump";
    else if (compression == 0x11) 
        extension = "pbm";
    else if (compression == 0x13) 
        extension = "jbg";
    else {
        err << QString(_("Page: Unsupported compression algorithm (0x%1)")).
            arg(compression, 0, 16) << endl;
        return false;
    }

    // Check the sub-header version
    if (_version > 3) {
        err << QString(_("Page: unsupported band sub-header version (%1)")).
            arg(_version) << endl;
        return false;
    }

    // Open files if it has not be done
    if (!_openFiles(compression, extension, err))
        return false;

    // Extract the sub-header (if version >= 2)
    if (_version >= 2) {
        size = read32(content, 0);
        content.remove(0, 4);
        if (_version == 3) {
            state = read32(content, 0);
            for (unsigned int i=0; i < 5; i++) {
                quint32 tmp = read32(content, (i + 1) * 4);

                if (tmp != 0)
                    err << QString(_("Page: invalid header value in sub-header "
                        "band (%1)")).arg(tmp) << endl;
            }
            content.remove(0, 6*4);
        }
        if (content.size() != size) {
            err << QString(_("Page: content data is %1 bytes long whereas it "
                "should be %2 bytes long")).arg(content.size()).arg(size) << 
                endl;
            return false;
        }
    }

    if (compression == 0)
        _dump(color-1, width, height, content, err);
    else if (compression == 0x13)
        _dump(color-1, width, height, content, err);
    else if (compression == 0x11)
        _compression0x11(color-1, width, height, content, err);

    return true;
}

void Page::flush()
{
    // Save the PBM file
    if (_lastBand[0] || _lastBand[1] || _lastBand[2] || _lastBand[3]) {
        for (unsigned int c=0; c < _maxColors; c++) {
            QString header = QString("P4\n# Page %1 - layer %2 --- (C) "
                "decompress, 2006-2007 by Aurélien Croc (AP²C)\n%3 %4\n").
                arg(_qpdl->page()).arg(c+1).arg(_width[c]).arg(_height[c]);

            _files[c].write(header.toLatin1());
            _files[c].write(_layers[c]);
        }
    }
    _closeFiles();
}



/*
 * Algorithmes de décompression
 * Decompress algorithms
 */
bool Page::_dump(quint8 color, quint16 width, quint16 height, QByteArray& data, 
        QTextStream& err)
{
    _files[color].write(data);
}

bool Page::_compression0x11(quint8 color, quint16 width, quint16 height, 
    QByteArray& data, QTextStream& err)
{
    QByteArray band;

    _width[color] = width;
    _height[color] += height;

    if (_lastBand[color] < _qpdl->currentBandNr()) {
        QByteArray blank((width * height + 7) >> 3, 0);

        while (_lastBand[color] < _qpdl->currentBandNr()) {
            _layers[color].append(blank);
            _lastBand[color]++;
        }
    }

    if (!decompress0x11(data, band, width, height, this, err))
        return false;
    _layers[color].append(band);
    _lastBand[color]++;
    return true;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8 : */

