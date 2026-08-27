/*
 *      qpdl.cpp                (C) 2006-2007, Aurélien Croc (AP²C)
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
#include "qpdl.h"
#include <QtCore/QFile>
#include <QtCore/QByteArray>
#include <QtCore/QTextStream>
#include "i18n.h"

/*
 * Définitions et déclarations
 * Definitions and declarations
 */
#define HEADER_SIGNATURE    0x00
#define HEADER_YRESOLUTION  0x01
#define HEADER_NRCOPIES_H   0x02
#define HEADER_NRCOPIES_L   0x03
#define HEADER_PAPERTYPE    0x04
#define HEADER_WIDTH_H      0x05
#define HEADER_WIDTH_L      0x06
#define HEADER_HEIGHT_H     0x07
#define HEADER_HEIGHT_L     0x08
#define HEADER_PAPERSOURCE  0x09
#define HEADER_UNKNOWN1	    0x0A
#define HEADER_DUPLEX       0x0B
#define HEADER_TUMBLE       0x0C
#define HEADER_UNKNOWN2	    0x0D
#define HEADER_QPDLVERSION  0x0E
#define HEADER_UNKNOWN3     0x0F
#define HEADER_XRESOLUTION  0x10

#define BAND_NUMBER         0x0
#define BAND_WIDTH_H        0x1
#define BAND_WIDTH_L        0x2
#define BAND_HEIGHT_H       0x3
#define BAND_HEIGHT_L       0x4

#define MAX_PAPER_TYPE 24
static const char *_paperTypeName[] = {
    "Letter",       // 0
    "Legal",
    "A4",
    "Executive",
    "Ledger",
    "A3",           // 5
    "Com10",
    "Monarch",
    "C5",
    "DL",
    "JB4",          // 10
    "JB5",
    "B5",
    "Not listed",
    "JPost",
    "JDouble",      // 15
    "A5",
    "A6",
    "JB6",
    "*Unknown*",
    "*Unknown*",    // 20
    "Custom",
    "*Unknown",
    "C6",
    "Folio"};       // 24

#define MAX_PAPER_SOURCE 7
static const char *_paperSourceName[] = {
    "*Unknown*",    // 0
    "Auto",
    "Manual",
    "Multi",
    "Top",
    "Lower",        // 5
    "Envelopes",
    "Third"};       // 7



/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
QPDLDocument::QPDLDocument()
{
    _pageNr = 0;
}



/*
 * Lecture de l'en-tête d'une page
 * Read page header
 */
QPDLDocument::Result QPDLDocument::_readPageHeader(QFile& data,
    QTextStream& out, QTextStream& err)
{
    QByteArray header = data.read(0x11);

    // Check header signature
    if (header.at(HEADER_SIGNATURE) == '\t')
        return End;
    if (header.at(HEADER_SIGNATURE) != 0) {
        err << QString(_("QPDL: bad page header signature (%1)")).
            arg((quint8)header.at(HEADER_SIGNATURE)) << endl;
        return Error;
    }

    // Resolution
    _resolutionX = header.at(HEADER_XRESOLUTION) ?
        (quint8)header.at(HEADER_XRESOLUTION) * 100 :
        (quint8)header.at(HEADER_YRESOLUTION) * 100;
    _resolutionY = (quint8)header.at(HEADER_YRESOLUTION) * 100;

    // Paper type
    _paperType = header.at(HEADER_PAPERTYPE);

    // Paper source
    _paperSource = header.at(HEADER_PAPERSOURCE);

    // Printable area size
    _width = ((quint8)header.at(HEADER_WIDTH_H) << 8) +
        (quint8)header.at(HEADER_WIDTH_L);
    _height = ((quint8)header.at(HEADER_HEIGHT_H) << 8) +
        (quint8)header.at(HEADER_HEIGHT_L);

    // Copies number
    _nrCopies = ((quint8)header.at(HEADER_NRCOPIES_H) << 8) +
        (quint8)header.at(HEADER_NRCOPIES_L);

    // Duplex and Tumble
    _duplex = header.at(HEADER_DUPLEX);
    _tumble = header.at(HEADER_TUMBLE);

    // QPDL VERSION
    _qpdl = header.at(HEADER_QPDLVERSION);

    // Unknown bytes
    _unknown[0] = header.at(HEADER_UNKNOWN1);
    _unknown[1] = header.at(HEADER_UNKNOWN2);
    _unknown[2] = header.at(HEADER_UNKNOWN3);

    // Print a summary
    if (!_quiet) {
        out << QString(_("Page %1 header:")).arg(_pageNr) << endl;
        out << "    " << QString(_("QPDL version..... = %1")).
            arg(_qpdl) << endl;
        out << "    " << QString(_("Number of copies. = %1")).
            arg(_nrCopies) << endl;
        out << "    " << QString(_("Resolution....... = %1×%2")).
            arg(_resolutionX).arg(_resolutionY) << endl;
        out << "    " << QString(_("Paper type....... = %1")).
            arg(_paperType <= MAX_PAPER_TYPE ? _paperTypeName[_paperType] :
            QString("*Unknown* (%1)").arg(_paperType)) << endl;
        out << "    " << QString(_("Paper source..... = %1")).
            arg(_paperSource<=MAX_PAPER_SOURCE ? _paperSourceName[_paperSource]:
            QString("*Unknown* (%1)").arg(_paperSource)) << endl;
        out << "    " << QString(_("Printable area... = %1×%2")).
            arg(_width).arg(_height) << endl;
        out << "    " << QString(_("Duplex - Tumble.. = %1 %2")).
            arg(_duplex, 1).arg(_tumble, 1) << endl;
        out << "    " << QString(_("Unknown bytes.... = %1 %2 %3")).
            arg((quint8)_unknown[0], 1).arg((quint8)_unknown[1], 1).
            arg((quint8)_unknown[2], 1) << endl;
    }

    return Ok;
}



/*
 * Extraction de chaques bandes
 * Band extraction
 */
