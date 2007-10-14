/*
 * 	spl2.cpp		(C) 2006, Aurélien Croc (AP²C)
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
 * ---
 *  Thanks to Keith White for his modifications, corrections and adds.
 * 
 */
#include "spl2.h"
#include "printer.h"
#include "document.h"
#include "error.h"
#include "band.h"
#include "bandanalyser.h"
#include <inttypes.h>


/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
SPL2::SPL2()
{
    _printer = NULL;
    _output = NULL;
}

SPL2::~SPL2()
{
}



/* 
 * Génération de l'en-tête et du pied de page PJL
 * Write PJL header and footer
 */
int SPL2::beginDocument()
{
    if (!_output || !_printer) {
        ERROR(_("SPL2::beginDocument: called with NULL parameters"));
        return -1;
    }

    _printer->newJob(_output);
    fprintf(_output, "@PJL ENTER LANGUAGE = QPDL\n");
    DEBUG("Envoie de l'en-tête du document JPL");
    return 0;
}

int SPL2::closeDocument()
{
    if (!_output || !_printer) {
        ERROR(_("SPL2::closeDocument: called with NULL parameters"));
        return -1;
    }
    _printer->endJob(_output);
    DEBUG("Envoie du pied de page du document JPL");
    return 0;
}



/*
 * Impression d'une page
 * Impress a page
 */
int SPL2::_writeColorBand(Band *band, int color)
{
    unsigned char *data, header[5];
    uint32_t checksum = 0;
    size_t size;

    // Compress
    if (!(data = band->exportBand(_printer->compVersion(), &size)))
        return 1;

    // Calculate the checksum
    if (_printer->qpdlVersion() != 0) {
        for (unsigned int j = 0; j < size; j++)
            checksum += data[j];
    }

    // Write the color header
    if (color) {
        header[0x0] = color;
        fwrite((char *)&header, 1, 1, _output);
    }
    header[0x0] = _printer->compVersion();      // Compression
    if (_printer->qpdlVersion() == 0) {
        header[0x1] = size >> 24;                // data length
        header[0x2] = size >> 16;                // data length
        header[0x3] = size >> 8;                // data length
        header[0x4] = size;                     // data length
    } else {
        header[0x1] = (size + 4) >> 24;          // data length
        header[0x2] = (size + 4) >> 16;          // data length
        header[0x3] = (size + 4) >> 8;          // data length
        header[0x4] = (size + 4);               // data length
    }
    fwrite((char *)&header, 1, 0x5, _output);

    // Write the data
    fwrite(data, 1, size, _output);
    delete[]data;

    // Write the checksum
    if (_printer->qpdlVersion() != 0) {
        header[0x0] = checksum >> 24;
        header[0x1] = checksum >> 16;
        header[0x2] = checksum >> 8;
        header[0x3] = checksum;
        fwrite((char *)&header, 1, 0x4, _output);
    }

    return 0;
}

int SPL2::printPage(Document *document, unsigned long nrCopies)
{
    unsigned long width, height, clippingX, clippingY;
    unsigned long bandNumber;
    unsigned long i;
    char header[0x11];
    char errors = 0;
    Band *bandC, *bandM, *bandY, *bandB;

    if (!document) {
        ERROR(_("SPL2::printPage: called with NULL parameter"));
        return -2;
    }
    // Load a new page
    if (document->loadPage(_printer))
        return -1;
    if (!document->height())
        return -1;


    // Send page header FIXME
    header[0x0] = 0;                                // Signature
    header[0x1] = _printer->resolutionY() / 100;    // Y Resolution
    header[0x2] = nrCopies >> 8;                    // Number of copies 8-15
    header[0x3] = nrCopies;                         // Number of copies 0-7
    header[0x4] = _printer->paperType();            // Paper type
    header[0x5] = document->width() >> 8;           // Printable area width
    header[0x6] = document->width();                // Printable area width
    header[0x7] = document->height() >> 8;          // Printable area height
    header[0x8] = document->height();               // Printable area height
    header[0x9] = _printer->paperSource();          // Paper source
    header[0xa] = _printer->docHeaderValues(0);     // ??? XXX
    header[0xb] = _printer->duplex() >> 8;          // Duplex
    header[0xc] = _printer->duplex();               // Duplex
    header[0xd] = _printer->docHeaderValues(1);     // ??? XXX
    header[0xe] = _printer->qpdlVersion();          // QPDL Version
    header[0xf] = _printer->docHeaderValues(2);     // ??? XXX
    if (_printer->resolutionY() != _printer->resolutionX() || 
        _printer->docHeaderValues(2) == 1)
        header[0x10] = _printer->resolutionX() / 100; // X Resolution
    else
        header[0x10] = 0;                           // X Resolution = Y Res.
    fwrite((char *)&header, 1, sizeof(header), _output);



    // Get the width, height, clipping X and clipping Y values
    if (document->width() <= _printer->areaX()) {
        clippingX = 0;
        width = document->width();
    } else if (document->width() < _printer->pageSizeX()) {
        clippingX = (unsigned long)(document->width() - _printer->areaX());
        width = document->width();
    } else {
        clippingX = (unsigned long)_printer->marginX();
        width = (unsigned long)_printer->pageSizeX();
    }
    if (document->height() <= _printer->areaY()) {
        clippingY = 0;
        height = document->height();
    } else if (document->height() < _printer->pageSizeY()) {
        clippingY = (unsigned long)(document->height() - _printer->areaY());
        height = document->height();
    } else {
        clippingY = (unsigned long)_printer->marginY();
        height = (unsigned long)_printer->pageSizeY();
    }


    // Create the band instance
    bandB = new Band((unsigned long)_printer->pageSizeX(),
        (unsigned long)_printer->bandHeight());
    if (document->isColor()) {
        bandC = new Band((unsigned long)_printer->pageSizeX(),
            (unsigned long)_printer->bandHeight());
        bandM = new Band((unsigned long)_printer->pageSizeX(),
            (unsigned long)_printer->bandHeight());
        bandY = new Band((unsigned long)_printer->pageSizeX(),
            (unsigned long)_printer->bandHeight());
    }
    bandNumber = 0;
    bandB->setClipping(clippingX);
    if (document->isColor()) {
        bandC->setClipping(clippingX);
        bandM->setClipping(clippingX);
        bandY->setClipping(clippingX);
    }

    // Clip vertically the document
    if (!document->isColor())
        clippingY = clippingY * 4;
    for (; clippingY; clippingY--)
        document->readLine();

    // Round up height to a multiple of bandHeight
    height += _printer->bandHeight() - (height % _printer->bandHeight());




    /*
     * Read and create each band
     */
    for (i = 0; i < height; i++) {
        unsigned long res;

        if (document->isColor()) {
            res = document->readLine();
            if (res < 0) {
                errors = 1;
                break;
            } else if (!res)
                break;
            if (bandC->addLine(document->lineBuffer(), (res > width ? width : 
                    res))) {
                errors = 1;
                break;
            }
            res = document->readLine();
            if (res < 0) {
                errors = 1;
                break;
            } else if (!res)
                break;
            if (bandM->addLine(document->lineBuffer(), (res > width ? width : 
                    res))) {
                errors = 1;
                break;
            }
            res = document->readLine();
            if (res < 0) {
                errors = 1;
                break;
            } else if (!res)
                break;
            if (bandY->addLine(document->lineBuffer(), (res > width ? width : 
                    res))) {
                errors = 1;
                break;
            }
        }
        res = document->readLine();
        if (res < 0) {
            errors = 1;
            break;
        } else if (!res)
            break;
        if (bandB->addLine(document->lineBuffer(), (res > width ? width : 
                res))) {
            errors = 1;
            break;
        }
        // Compress and send the band if it's complete
        if (bandB->isFull()) {
            if (document->isColor())
                correctBlackColor(bandC, bandM, bandY, bandB);
            checkEmptyBand(bandB);
            if (document->isColor()) {
                checkEmptyBand(bandC);
                checkEmptyBand(bandM);
                checkEmptyBand(bandY);
            }
            if ((!document->isColor() && bandB->isEmpty()) ||
                    (document->isColor() && bandB->isEmpty() &&
                    bandC->isEmpty() && bandM->isEmpty() && bandY->isEmpty())) {
                bandNumber++;
                bandB->clean();
                if (document->isColor()) {
                    bandC->clean();
                    bandM->clean();
                    bandY->clean();
                }
                continue;
            }
            // QPDL Version 0 or 1
            if (_printer->qpdlVersion() < 2) {
                // Write the band header
                header[0x0] = 0xC;                  // Signature
                header[0x1] = bandNumber;           // Band number
                header[0x2] = bandB->width() >> 8;  // Band width
                header[0x3] = bandB->width();       // Band width
                header[0x4] = bandB->height() >> 8; // Band height
                header[0x5] = bandB->height();      // Band height
                fwrite((char *)&header, 1, 0x6, _output);

                if (_printer->isColorPrinter()) {
                    if (document->isColor()) {
                        if (!bandC->isEmpty()) {
                            if (_writeColorBand(bandC, 1)) {
                                errors = 1;
                                break;
                            }
                        }
                        if (!bandM->isEmpty()) {
                            if (_writeColorBand(bandM, 2)) {
                                errors = 1;
                                break;
                            }
                        }
                        if (!bandY->isEmpty()) {
                            if (_writeColorBand(bandY, 3)) {
                                errors = 1;
                                break;
                            }
                        }
                    }
                    if (!bandB->isEmpty()) {
                        if (_writeColorBand(bandB, 4)) {
                            errors = 1;
                            break;
                        }
                    }
                    header[0x0] = 0;                // End color
                    fwrite((char *)&header, 1, 1, _output);
                } else {
                    if (_writeColorBand(bandB, 0)) {
                        errors = 1;
                        break;
                    }
                }

            // QPDL Version 2
            } else if (_printer->qpdlVersion() == 2) {
                if (_printer->isColorPrinter()) {
                    if (!bandB->isEmpty()) {
                        // Write the band header
                        header[0x0] = 0xC;                  // Signature
                        header[0x1] = bandNumber;           // Band number
                        header[0x2] = bandB->width() >> 8;  // Band width
                        header[0x3] = bandB->width();       // Band width
                        header[0x4] = bandB->height() >> 8; // Band height
                        header[0x5] = bandB->height();      // Band height
                        fwrite((char *)&header, 1, 0x6, _output);
                        if (_writeColorBand(bandB, 4)) {
                            errors = 1;
                            break;
                        }
                    }
                    if (document->isColor()) {
                        if (!bandC->isEmpty()) {
                            // Write the band header
                            header[0x0] = 0xC;                  // Signature
                            header[0x1] = bandNumber;           // Band number
                            header[0x2] = bandC->width() >> 8;  // Band width
                            header[0x3] = bandC->width();       // Band width
                            header[0x4] = bandC->height() >> 8; // Band height
                            header[0x5] = bandC->height();      // Band height
                            fwrite((char *)&header, 1, 0x6, _output);
                            if (_writeColorBand(bandC, 1)) {
                                errors = 1;
                                break;
                            }
                        }
                        if (!bandM->isEmpty()) {
                            // Write the band header
                            header[0x0] = 0xC;                  // Signature
                            header[0x1] = bandNumber;           // Band number
                            header[0x2] = bandM->width() >> 8;  // Band width
                            header[0x3] = bandM->width();       // Band width
                            header[0x4] = bandM->height() >> 8; // Band height
                            header[0x5] = bandM->height();      // Band height
                            fwrite((char *)&header, 1, 0x6, _output);
                            if (_writeColorBand(bandM, 2)) {
                                errors = 1;
                                break;
                            }
                        }
                        if (!bandY->isEmpty()) {
                            // Write the band header
                            header[0x0] = 0xC;                  // Signature
                            header[0x1] = bandNumber;           // Band number
                            header[0x2] = bandY->width() >> 8;  // Band width
                            header[0x3] = bandY->width();       // Band width
                            header[0x4] = bandY->height() >> 8; // Band height
                            header[0x5] = bandY->height();      // Band height
                            fwrite((char *)&header, 1, 0x6, _output);
                            if (_writeColorBand(bandY, 3)) {
                                errors = 1;
                                break;
                            }
                        }
                    }
                } else {
                    // Write the band header
                    header[0x0] = 0xC;                  // Signature
                    header[0x1] = bandNumber;           // Band number
                    header[0x2] = bandB->width() >> 8;  // Band width
                    header[0x3] = bandB->width();       // Band width
                    header[0x4] = bandB->height() >> 8; // Band height
                    header[0x5] = bandB->height();      // Band height
                    fwrite((char *)&header, 1, 0x6, _output);
                    if (_writeColorBand(bandB, 0)) {
                        errors = 1;
                        break;
                    }
                }
            }
            bandNumber++;
            bandB->clean();
            if (document->isColor()) {
                bandC->clean();
                bandM->clean();
                bandY->clean();
            }
        }
    }



    // Clean everything
    delete bandB;

    if (document->isColor()) {
        bandC->clean();
        bandM->clean();
        bandY->clean();
    }
    if (errors)
        return -11;

    // Write the end of the page
    header[0x0] = 1;                        // Signature
    header[0x1] = nrCopies >> 8;            // Number of copies 8-15
    header[0x2] = nrCopies;                 // Number of copies 0-7
    fwrite((char *)&header, 1, 3, _output);

    return 0;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin: */
