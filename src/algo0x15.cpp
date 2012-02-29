/*
 * 	    algo0x15.cpp              (C) 2006-2008, Aurélien Croc (AP²C)
 *                                    This file is a SpliX derivative work
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
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  $Id: algo0x15.cpp 287 2011-02-19 19:10:22Z tillkamppeter $
 * --
 * This code was written by Leonardo Hamada
 * 
 */
#include "algo0x15.h"
#include <string.h>
#include "errlog.h"
#include "request.h"
#include "printer.h"
#include "bandplane.h"

#ifndef DISABLE_JBIG

extern "C" {
#include "jbig85.h"
}

/*
 * Fonction de rappel
 * Callback
 */
void Algo0x15::_callback(unsigned char *data, size_t data_len, void *arg)
{
    Algo0x15 *compressor = (Algo0x15 *)arg;
    if (!data_len) {
        compressor->_error = true;
        return;
    }
    if ((!compressor->_has_bih) && (0 == compressor->_size)) {
        if (20 != data_len) {
            ERRORMSG(_("Expected 20 bytes from BIH (0x15)"));
            compressor->_error = true;
            return;
        }
        memcpy(compressor->_bih, data, 20);
        compressor->_has_bih = true;
    } else {  
        unsigned long freeSpace = compressor->_maxSize - compressor->_size;
        if (data_len > freeSpace) {
            ERRORMSG(_("Insufficient buffer space to store BIE (0x15)"));
            compressor->_error = true;
            return;
        }
        memcpy(compressor->_data + compressor->_size, data, data_len);
        compressor->_size += data_len;
    }
}

/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
Algo0x15::Algo0x15()
{
    _has_bih = false;
    _data = NULL;
    _size = 0;
    _maxSize = 0;
    _error = false;
}

Algo0x15::~Algo0x15()
{
    if (_data)
        delete [] _data;
}

/*
 * Routine de compression
 * Compression routine
 * The nature of his compression scheme is to have an entire page divided into
 * bands, each band is compressed with the JBIG to be assembled by the printer.
 * This method is called for each band returning corresponding JBIG data.
 * Assumes compressed band data fits in the space specified
 * in the printer PPD file: QPDL PacketSize: "512", specifies 512 Kbytes limit.
 */
BandPlane* Algo0x15::compress(const Request& request, unsigned char *data, 
        unsigned long width, unsigned long height)
{
    #define MAX_SIZE 512 * 1024
    BandPlane *plane; 
    jbg85_enc_state state;
    unsigned long wbytes;
    if (!data || !width || !height) {
        ERRORMSG(_("Invalid given data for compression (0x15)"));
        return NULL;
    }
    if (_has_bih)
        _has_bih = false;
    if (_size)
        _size = 0;
    if (_error)
        _error = false;
    if (0 == _maxSize)
        _maxSize = request.printer()->packetSize();
    if ((!_maxSize) || (_maxSize > MAX_SIZE)) {
        ERRORMSG(_("PacketSize is set to %luBytes! Reset to %dBytes."),
                                                   _maxSize, MAX_SIZE);
        _maxSize = MAX_SIZE;
    }
    if (NULL == _data)
        _data = new unsigned char[_maxSize];
    wbytes = (width + 7) / 8;
    jbg85_enc_init(&state, width, height, _callback, this);
    jbg85_enc_options(&state, JBG_LRLTWO, height, 0);
    for (unsigned long i = 0; i < height; i++) {
        jbg85_enc_lineout(&state,
                          data + i * wbytes,
                          data + (i - 1) * wbytes,
                          data + (i - 2) * wbytes);
    }
    if (_error)
        return NULL;
    unsigned char *final_data = new unsigned char[_size];
    memcpy(final_data, _data, _size);
    plane = new BandPlane();
    plane->setCompression(0x15);
    plane->setEndian(BandPlane::BigEndian);
    plane->setData(final_data, _size);
    /* Finished encoding of this band. */
    DEBUGMSG(_("Band encoded with type=0x15, size=%lu"), _size);
    /* Clean up. */
    _has_bih = false;
    _size = 0;
    if (_error)
        _error = false;
    return plane;
}

#endif /* DISABLE_JBIG */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

