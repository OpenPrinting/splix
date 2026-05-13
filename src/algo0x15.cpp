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
#include <cstring>
#include <vector>
#include <memory>
#include <span>
#include <cstdint>
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
    Algo0x15 *compressor = static_cast<Algo0x15 *>(arg);
    if (!data_len) {
        compressor->_error = true;
        return;
    }
    if ((!compressor->_has_bih) && compressor->_data.empty()) {
        if (20 != data_len) {
            ERRORMSG(_("Expected 20 bytes from BIH (0x15)"));
            compressor->_error = true;
            return;
        }
        std::ranges::copy(std::span(data, 20), compressor->_bih.begin());
        compressor->_has_bih = true;
    } else {  
        if (compressor->_data.size() + data_len > compressor->_maxSize) {
            ERRORMSG(_("Insufficient buffer space to store BIE (0x15)"));
            compressor->_error = true;
            compressor->_errorCode = SP::Error::CompressionError;
            return;
        }
        compressor->_data.insert(compressor->_data.end(), data, data + data_len);
    }
}

/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
Algo0x15::Algo0x15()
{
}

// Destructor is defaulted in header

/*
 * Routine de compression
 * Compression routine
 * The nature of his compression scheme is to have an entire page divided into
 * bands, each band is compressed with the JBIG to be assembled by the printer.
 * This method is called for each band returning corresponding JBIG data.
 * Assumes compressed band data fits in the space specified
 * in the printer PPD file: QPDL PacketSize: "512", specifies 512 Kbytes limit.
 */
SP::Result<std::unique_ptr<BandPlane>> Algo0x15::compress([[maybe_unused]] const Request& request, 
                                     std::span<const uint8_t> data, uint32_t width,
                                     uint32_t height)
{
    const uint32_t MAX_SIZE_LIMIT = 512 * 1024;
    jbg85_enc_state state;
    uint32_t wbytes;

    // Input sanitization
    if (width > 32768 || height > 32768 || width == 0 || height == 0) {
        ERRORMSG(_("Invalid raster dimensions for Algo0x15: %ux%u"), width, height);
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    if (data.empty()) {
        ERRORMSG(_("Invalid given data for compression (0x15)"));
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    _has_bih = false;
    _data.clear();
    _error = false;
    _errorCode = SP::Error::None;

    if (0 == _maxSize) {
        if (!request.printer()) {
            return SP::Unexpected(SP::Error::InvalidArgument);
        }
        _maxSize = static_cast<uint32_t>(request.printer()->packetSize());
    }
    
    if ((!_maxSize) || (_maxSize > MAX_SIZE_LIMIT)) {
        ERRORMSG(_("PacketSize is set to %u Bytes! Reset to %u Bytes."),
                                                     _maxSize, MAX_SIZE_LIMIT);
        _maxSize = MAX_SIZE_LIMIT;
    }
    
    try {
        _data.reserve(_maxSize);
    } catch (const std::bad_alloc&) {
        return SP::Unexpected(SP::Error::MemoryError);
    }

    // Defensive check: prevent overflow in wbytes calculation
    if (width > 0x1FFFFFFF) {
         return SP::Unexpected(SP::Error::RasterDimensionTooLarge);
    }
    wbytes = (width + 7) / 8;

    // Ensure data span is large enough
    if (data.size() < static_cast<size_t>(wbytes) * height) {
        ERRORMSG(_("Data span too small for dimensions: %zu < %u*%u"), 
                 data.size(), wbytes, height);
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    jbg85_enc_init(&state, width, height, _callback, this);
    jbg85_enc_options(&state, JBG_LRLTWO, height, 0);
    for (uint32_t i = 0; i < height; i++) {
        const uint8_t* curr = &data[i * wbytes];
        const uint8_t* prev = (i > 0) ? &data[(i - 1) * wbytes] : nullptr;
        const uint8_t* prev2 = (i > 1) ? &data[(i - 2) * wbytes] : nullptr;

        /* JBIG85 C API requires non-const pointers; we cast here as we manage 
           the buffer lifecycle during this call. */
        jbg85_enc_lineout(&state,
                          const_cast<unsigned char*>(curr),
                          const_cast<unsigned char*>(prev),
                          const_cast<unsigned char*>(prev2));
        
        if (_error) {
             break;
        }
    }

    if (_error) {
        return SP::Unexpected(_errorCode != SP::Error::None ? _errorCode : SP::Error::LogicError);
    }

    auto plane = std::make_unique<BandPlane>();
    plane->setCompression(0x15);
    plane->setEndian(BandPlane::Endian::BigEndian);
    plane->setData(std::move(_data));

    /* Finished encoding of this band. */
    DEBUGMSG(_("Band encoded with type=0x15, size=%zu"), plane->dataSize());
    
    return plane;
}

#endif /* DISABLE_JBIG */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

