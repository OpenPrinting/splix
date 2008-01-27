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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
        header.at(HEADER_WIDTH_L);
    _height = ((quint8)header.at(HEADER_HEIGHT_H) << 8) + 
        header.at(HEADER_HEIGHT_L);

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
QPDLDocument::Result QPDLDocument::_readPageContent(QFile& data, 
    QTextStream& out, QTextStream& err)
{
    if (!_quiet) {
        if (_decompression)
            out << _("    Analysing and decompression");
        else
            out << _("    Analysing");
    }

    while (!data.atEnd()) {
        QByteArray header, content;
        quint16 width, height;
        char signature, color=1;
        quint8 compression;
        quint32 size;

        // Check the band signature
        data.getChar(&signature);
        if (signature == 0x1) { // End of the page
            if (!_quiet)
                out << QString("(0x%1)").arg(compression, 0, 16) << endl;
            return Ok;
        } else if (signature != 0xC) {
            err << QString(_("QPDL: bad band header signature (%1)")).
                arg((quint8)signature) << endl;
            return Error;
        }

        // Extract the band information
        header = data.read(5);
        _currentBandNr = header.at(BAND_NUMBER);
        width = ((quint8)header.at(BAND_WIDTH_H) << 8) + 
            (quint8)header.at(BAND_WIDTH_L);
        height = ((quint8)header.at(BAND_HEIGHT_H) << 8) + 
            (quint8)header.at(BAND_HEIGHT_L);
        if (!_quiet)
            out << '.';

        // Read the band data -- SPLc in QPDL V. 0 or 1
        if (_type == SPLc && _qpdl <= 1) {
            while (!data.atEnd()) {
                // Read the color
                data.getChar(&color);
                if (!color)
                    break;
                if (color < 0 || color > 4) {
                    err << QString(_("QPDL: bad color value (%1) in band %2")).
                        arg((quint8)color).arg(_currentBandNr) << endl;
                    return Error;
                }

                // Read the compression and the size
                header = data.read(5);
                compression = header.at(0);
                size = ((quint8)header.at(1) << 24) + 
                    ((quint8)header.at(2) << 16) + 
                    ((quint8)header.at(3) << 8) + (quint8)header.at(4);
                content = data.read(size);

                // Process the band
                if (!_processBandAnalysis(color, width, height, compression, 
                    content, out, err))
                    return Error;
            } 


        // Read the band data -- SPLc in QPDL V. 2
        } else if (_type == SPLc && _qpdl == 2) {
            // Read the color
            data.getChar(&color);
            if (color <= 0 || color > 4) {
                err << QString(_("QPDL: bad color value (%1) in band %2")).
                    arg((quint8)color).arg(_currentBandNr) << endl;
                return Error;
            }

            // Read the compression and the size
            header = data.read(5);
            compression = header.at(0);
            size = ((quint8)header.at(1) << 24) + ((quint8)header.at(2) << 16) +
                ((quint8)header.at(3) << 8) + (quint8)header.at(4);
            content = data.read(size);

            // Process the band
            if (!_processBandAnalysis(color, width, height, compression, 
                        content, out, err))
                return Error;


        // Read the band data -- SPL-2 in any QPDL
        } else {
            // Read the compression and the size
            header = data.read(5);
            compression = header.at(0);
            size = ((quint8)header.at(1) << 24) + ((quint8)header.at(2) << 16) +
                ((quint8)header.at(3) << 8) + (quint8)header.at(4);
            content = data.read(size);

            // Process the band
            if (!_processBandAnalysis(1, width, height, compression, 
                        content, out, err))
                return Error;
        }
    }
    return Error;
}



/*
 * Traitement d'une bande
 * Band management
 */
bool QPDLDocument::_processBandAnalysis(quint8 color, quint16 width, 
    quint16 height, quint8 compression, QByteArray& content,
    QTextStream& out, QTextStream& err)
{
    quint32 checkSum = 0, givenCheckSum;
    quint8 version;
    bool be;

    // Check the signature and the BE/LE
    if (((quint8)content.at(0) & 0xF) != 0x9 || (quint8)content.at(1) != 0xAB ||
        (quint8)content.at(2) != 0xCD || (quint8)content.at(3) != 0xEF) {
        if (((quint8)content.at(3) & 0xF) != 0x09 || 
            (quint8)content.at(2) != 0xAB || (quint8)content.at(1) != 0xCD || 
            (quint8)content.at(0) != 0xEF) {
            err << QString(_("QPDL: Invalid signature (0x%1%2%3%4)")).
                arg((quint8)content.at(0),0,16).arg((quint8)content.at(1),0,16).
                arg((quint8)content.at(2),0,16).arg((quint8)content.at(3),0,16) 
                << endl;
            return false;
        } else {
            be = false;
            version = (quint8)content.at(3) >> 4;
        }
    } else {
        be = true;
        version = (quint8)content.at(0) >> 4;
    }
    _page.setSubHeaderVersion(version);
    _page.setBE(be);

    // Check the checksum
    if (_qpdl > 0) {
        givenCheckSum = ((quint8)content.at(content.size() - 4) << 24) + 
            ((quint8)content.at(content.size() - 3) << 16) + 
            ((quint8)content.at(content.size() - 2) << 8) +
            (quint8)content.at(content.size() - 1);
        for (unsigned int i=0; i < content.size() - 4; i++)
            checkSum += (quint8)content.at(i);
        if (givenCheckSum != checkSum) {
            err << QString(_("QPDL: band checksum invalid! (0x%1-0x%2)")).
                arg(givenCheckSum, 8, 16, QChar('0')).arg(checkSum, 8, 16,
                QChar('0')) << endl;
            return false;
        }
        content.remove(content.size() - 5, 4);
    }
    content.remove(0, 4);

    // Decompress the band
    if (_decompression || _dump)
        return _page.process(color, width, height, _dump ? 0 : compression, 
            content, out, err);
    return true;
}



/* 
 * Boucle générale de traitement des données
 * Data management main loop
 */
bool QPDLDocument::parse(QFile& data, QTextStream& out, QTextStream& err)
{
    _page.setQPDLDocument(this);

    while (!data.atEnd()) {
        QByteArray footer;
        quint16 nrCopies;
        Result res;

        _pageNr++;
        _page.clear();

        // Read the page header
        res = _readPageHeader(data, out, err);
        if (res == Error)
            return false;
        if (res == End)
            break;

        // Read the page bands
        res = _readPageContent(data, out, err);
        if (res == Error)
            return false;
        if (res == End)
            break;

        // Read the page footer
        footer = data.read(2);
        nrCopies = ((quint8)footer.at(0) << 8) + (quint8)footer.at(1);
        if (!_quiet)
            out << QString(_("Page %1 done (%2 copie(s)).")).arg(_pageNr).
                arg(nrCopies) << endl << endl;
        _page.flush();
    }
    return true;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8 : */

