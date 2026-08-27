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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
    _version = 0;
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
