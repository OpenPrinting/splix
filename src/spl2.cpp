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
 *  The JBIG algorithm has been inspired from the Rick Richardson's work.
 * 
 */
#include "spl2.h"
#include "printer.h"
#include "document.h"
#include "error.h"
#include "band.h"
#include "bandanalyser.h"
#include <inttypes.h>
#include <string.h>
extern "C" {
#   include <jbig.h>
}

typedef struct compressedData_s {
    unsigned char*          data;
    unsigned long           size;
    compressedData_s*       next;
} compressedData_t;

typedef struct {
    compressedData_s*       next;
    compressedData_s*       last;
    unsigned long           packetSize;
} rootCompressedData_t;



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
    char header[0x11];
    char errors = 0;

    if (!document) {
        ERROR(_("SPL2::printPage: called with NULL parameter"));
        return -2;
    }
    // Load a new page
    if (document->loadPage(_printer))
        return -1;
    if (!document->height())
        return -1;
    fprintf(stderr, "PAGE: %lu %lu\n", document->page(), nrCopies);


    // Send page header
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

    // Clip vertically the document
    if (!document->isColor())
        clippingY = clippingY * 4;
    for (; clippingY; clippingY--)
        document->readLine();

    // Round up height to a multiple of bandHeight
    height += _printer->bandHeight() - (height % _printer->bandHeight());


    // Compress the page
    if (_printer->compVersion() <= 0x11)
        errors = _compressByBands(document, width, height, clippingX);
    else if (_printer->compVersion() == 0x13)
        errors = _compressByDocument(document, width, height, clippingX);

    if (errors)
        return -11;

    // Write the end of the page
    header[0x0] = 1;                        // Signature
    header[0x1] = nrCopies >> 8;            // Number of copies 8-15
    header[0x2] = nrCopies;                 // Number of copies 0-7
    fwrite((char *)&header, 1, 3, _output);

    return 0;
}

void callbackJBIGCompression(unsigned char *data, size_t len, void *arg)
{
    rootCompressedData_t **root = (rootCompressedData_t **)arg;
    compressedData_t *current;

    DEBUG("RECEPTION DATA = %lu\n", len);
    if (!*root) {
        ERROR(_("No root compression structure available"));
        return;
    }

    // Allocate the root structure
    if (!(*root)->last) {
//        (*root)->data = new unsigned char[len];
//        (*root)->size = len;
        (*root)->next = new compressedData_t;
        (*root)->next->data = new unsigned char[len];
        (*root)->next->size = len;
        (*root)->next->next = new compressedData_t;
        (*root)->next->next->data = NULL;
        (*root)->next->next->next = NULL;
        (*root)->last = (*root)->next->next;
        memcpy((*root)->next->data, data, len);
        if (len != 20)
            ERROR(_("JBIG Compression: the first BIH *MUST* be 20 bytes long "
                "(currently=%ld)\n"), len);
        return;
    }

    // Register data
    current = (*root)->last;
    while (len > 0) {
        unsigned long freeSpace, toWrite;

        // Create a new node if needed
        if (current->size == (*root)->packetSize) {
            current->next = new compressedData_t;
            current = current->next;
            current->data = NULL;
            current->next = NULL;
            (*root)->last = current;
        }

        // Create a buffer if needed
        if (!current->data) {
            current->data = new unsigned char[(*root)->packetSize];
            current->size = 0;
        }

        // Register compressed data in the current buffer
        freeSpace = (*root)->packetSize - current->size;
        toWrite = freeSpace < len ? freeSpace : len;
        memcpy(current->data + current->size, data, toWrite);
        current->size += toWrite;
        data += toWrite;
        len -= toWrite;
    }
}

static int _writeBE(unsigned long val, FILE *output)
{
    char header[4];

    header[0] = val >> 24;
    header[1] = val >> 16;
    header[2] = val >> 8;
    header[3] = val;
    fwrite((char *)&header, 1, 0x4, output);
    return header[0] + header[1] + header[2] + header[3];
}

int SPL2::_compressByDocument(Document *document, unsigned long width, 
    unsigned long height, unsigned long clippingX)
{
    compressedData_t *tmp, *cur[4] = {NULL, NULL, NULL, NULL};
    unsigned int colors = document->isColor() ? 4 : 1, checksum;
    struct jbg_enc_state layerState[4];
    unsigned int bandNumber=0, size;
    rootCompressedData_t *cdata[4];
    unsigned char *layer[4][1];
    unsigned long _width;
    char header[0x11];
    int errors=0;

    // Allocate the bitmap buffers
    width = width - clippingX;
    _width = (width + 7) / 8;
    DEBUG("width=%lu height=%lu, line=%lu", _width, height, document->lineSize());
    for (unsigned int c=0; c < colors; c++) {
        DEBUG("-- ON ALLOUE %lu OCTETS\n", _width * height);
        *layer[c] = new unsigned char[_width * height];
        cdata[c] = new rootCompressedData_t;
        cdata[c]->last = NULL;
        cdata[c]->packetSize = _printer->packetSize();
    }

    // Read each lines of each planes and store them in the bitmap buffers
    for (unsigned long i = 0; i < height; i++) {
        for (unsigned int c=0; c < colors; c++) {
            unsigned long start, res;
            unsigned char *line;

            if ((int)(res = document->readLine()) < 0) {
                errors = 1;
                break;
            }

            // Store the line and clip it if needed
            line = document->lineBuffer();
            start = _width < res ? res - _width : 0;
            for (unsigned long j = 0; j + start < res; j++)
                (*layer[c])[j] = line[j + start];
        }
    }

    // Compress each layer in JBIG
    for (unsigned int c=0; c < colors; c++) {
        jbg_enc_init(&layerState[c], width, height, 1, layer[c],
            callbackJBIGCompression,  &cdata[c]);
        jbg_enc_options(&layerState[c], 0, JBG_DELAY_AT | JBG_LRLTWO | 
            JBG_TPBON, height, 0, 0);
        jbg_enc_out(&layerState[c]);
        jbg_enc_free(&layerState[c]);
        delete *layer[c];
        cur[c] = cdata[c]->next;
    }

    // Export the result in the QPDL document
    while (cur[0] || cur[1] || cur[2] || cur[3]) {
        header[0x0] = 0xC;                  // Signature
        header[0x1] = bandNumber;           // Band number
        header[0x2] = width >> 8;           // Band width
        header[0x3] = width;                // Band width
        header[0x4] = _printer->bandHeight() >> 8; // Band height
        header[0x5] = _printer->bandHeight(); // Band height
        fwrite((char *)&header, 1, 0x6, _output);

        for (unsigned int c=0; c < colors; c++) {
            if (!cur[c])
                continue;

            size = cur[c]->size + 4+4+7*4;  // 4 for the BE/LE signature
                                            // 4 for the checksum
                                            // 7*4 for the 7 dword 
                                            //     JBIG header values
            if (colors == 4) {
                header[0x0] = c + 1;        // Color layer
                fwrite((char *)&header, 1, 0x1, _output);
            }
            header[0x0] = _printer->compVersion();      // Compression version
            fwrite((char *)&header, 1, 0x1, _output);
            checksum = _writeBE(size, _output);         // Size
            checksum += _writeBE(0x39ABCDEF, _output);  // Signature
            checksum += _writeBE(cur[c]->size, _output);// JBIG size
            if (!bandNumber)
                checksum += _writeBE(0, _output);
            else if (cur[c]->next)
                checksum += _writeBE(0x01000000, _output);
            else
                checksum += _writeBE(0x02000000, _output);
            _writeBE(0, _output);
            _writeBE(0, _output);
            _writeBE(0, _output);
            _writeBE(0, _output);
            _writeBE(0, _output);
            fwrite(cur[c]->data, 1, cur[c]->size, _output);
            for (unsigned int j=0; j < cur[c]->size; j++)
                checksum += cur[c]->data[j];
            _writeBE(checksum, _output);
            
            tmp = cur[c]->next;
            delete cur[c];
            cur[c] = tmp;
            bandNumber++;
        }
    }

    // Free the compression structures
    for (unsigned int c=0; c < colors; c++)
        delete cdata[c];

    return errors;
}

int SPL2::_compressByBands(Document *document, unsigned long width, 
    unsigned long height, unsigned long clippingX)
{
    Band *bandC, *bandM, *bandY, *bandB;
    unsigned long bandNumber;
    char header[0x11];
    unsigned long i;
    char errors = 0;

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
        delete bandC;
        delete bandM;
        delete bandY;
    }

    return errors;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

