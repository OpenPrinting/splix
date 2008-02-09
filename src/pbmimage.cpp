/*
 * 	pbmimage.cpp		(C) 2006, Aurélien Croc (AP²C)
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
#include "pbmimage.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "error.h"

/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
PbmImage::PbmImage(const char *black, const char *cyan, const char *magenta,
    const char *yellow)
{
    _color = !cyan && !magenta && !yellow ? false : true;
    _blackFile = black;
    _cyanFile = cyan;
    _magentaFile = magenta;
    _yellowFile = yellow;
    _black = NULL;
    _cyan = NULL;
    _magenta = NULL;
    _yellow = NULL;

    _width = 0;
    _height = 0;
    _lineSize = 0;
    _line = 0;
    _lineBuffer = NULL;
    _currentColor = 1;

}

PbmImage::~PbmImage()
{
    unload();
}



/*
 * Chargement de l'image
 * Load the image
 */
void PbmImage::unload()
{
    if (_black) {
        fclose(_black);
        _blackFile = NULL;
        _black = NULL;
    }
    if (_color) {
        if (_cyan) {
            fclose(_cyan);
            _cyanFile = NULL;
            _cyan = NULL;
        }
        if (_magenta) {
            fclose(_magenta);
            _magentaFile = NULL;
            _magenta = NULL;
        }
        if (_yellow) {
            fclose(_yellow);
            _yellowFile = NULL;
            _yellow = NULL;
        }
    }
    if (_lineBuffer) {
        delete[] _lineBuffer;
        _lineBuffer= NULL;
    }
}

int PbmImage::load()
{
    uint32_t width, height;
    char buffer[1024];

    // Open the different files
    if (!(_black = fopen(_blackFile, "r"))) {
        fprintf(stderr, _("Cannot open black file %s\n"), _blackFile);
        return -1;
    }
    if (_color) {
        if (_cyanFile && !(_cyan = fopen(_cyanFile, "r"))) {
            fprintf(stderr, _("Cannot open cyan file %s\n"), _cyanFile);
            return -1;
        }
        if (_magentaFile && !(_magenta = fopen(_magentaFile, "r"))) {
            fprintf(stderr, _("Cannot open magenta file %s\n"), _magentaFile);
            return -1;
        }
        if (_yellowFile && !(_yellow = fopen(_yellowFile, "r"))) {
            fprintf(stderr, _("Cannot open yellow file %s\n"), _yellowFile);
            return -1;
        }
    }

    // Read the PBM header
    fgets((char *)&buffer, sizeof(buffer), _black);
    if (strcmp((char *)&buffer, "P4\n")) {
        fprintf(stderr, _("Invalid PBM file for file %s\n"), _blackFile);
        return -1;
    }
    fgets((char *)&buffer, sizeof(buffer), _black);
    fscanf(_black, "%u %u\n", &width, &height);
    _width = width;
    _height = height;
    _lineSize = (width + 7) >> 3;

    if (_color) {
        unsigned int tmpW, tmpH;

        if (_cyan) {
            fgets((char *)&buffer, sizeof(buffer), _cyan);
            if (strcmp((char *)&buffer, "P4\n")) {
                fprintf(stderr, _("Invalid PBM file for file %s\n"), _cyanFile);
                return -1;
            }
            fgets((char *)&buffer, sizeof(buffer), _cyan);
            fscanf(_cyan, "%u %u\n", &tmpW, &tmpH);
            if ((tmpW != width) || (tmpH != height)) {
                fprintf(stderr, _("The different PBM layers must have the same "
                    "size\n"));
                return -1;
            }
        }
        if (_magenta) {
            fgets((char *)&buffer, sizeof(buffer), _magenta);
            if (strcmp((char *)&buffer, "P4\n")) {
                fprintf(stderr, _("Invalid PBM file for file %s\n"), 
                    _magentaFile);
                return -1;
            }
            fgets((char *)&buffer, sizeof(buffer), _magenta);
            fscanf(_magenta, "%u %u\n", &tmpW, &tmpH);
            if ((tmpW != width) || (tmpH != height)) {
                fprintf(stderr, _("The different PBM layers must have the same "
                    "size\n"));
                return -1;
            }
        }
        if (_yellow) {
            fgets((char *)&buffer, sizeof(buffer), _yellow);
            if (strcmp((char *)&buffer, "P4\n")) {
                fprintf(stderr, _("Invalid PBM file for file %s\n"), 
                    _yellowFile);
                return -1;
            }
            fgets((char *)&buffer, sizeof(buffer), _yellow);
            fscanf(_yellow, "%u %u\n", &tmpW, &tmpH);
            if ((tmpW != width) || (tmpH != height)) {
                fprintf(stderr, _("The different PBM layers must have the same "
                    "size\n"));
                return -1;
            }
        }
    }

    return 0;
}

int PbmImage::loadPage(Printer *printer)
{
    printer->setCompVersion(0x11);
    if (_line)
        return 1;

    return 0;
}



/* 
 * Lecture d'une ligne
 * Read a line
 */
int PbmImage::readLine()
{
    if (!_lineBuffer)
        _lineBuffer = new unsigned char[_lineSize];

    if (_line >= _height) {
        memset(_lineBuffer, 0x00, _lineSize);
        return _lineSize;
    }

    if (_color) {
        switch(_currentColor) {
            case 1:
                if (_cyan)
                    fread(_lineBuffer, 1, _lineSize, _cyan);
                else
                    memset(_lineBuffer, 0x00, _lineSize);
                _currentColor++;
                break;
            case 2:
                if (_magenta)
                    fread(_lineBuffer, 1, _lineSize, 
                            _magenta);
                else
                    memset(_lineBuffer, 0x00, _lineSize);
                _currentColor++;
                break;
            case 3:
                if (_yellow)
                    fread(_lineBuffer, 1, _lineSize, 
                            _yellow);
                else
                    memset(_lineBuffer, 0x00, _lineSize);
                _currentColor++;
                break;
            case 4:
                fread(_lineBuffer, 1, _lineSize, _black);
                _currentColor = 1;
                _line++;
                break;
        }
    } else {
        fread(_lineBuffer, 1, _lineSize, _black);
        _line++;
    }
    return _lineSize;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

