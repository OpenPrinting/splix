/*
 * 	    algo0x13.cpp              (C) 2006-2008, Aurélien Croc (AP²C)
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
 *  $Id$
 * 
 */
#include "algo0x13.h"
#include <cstring>
#include <vector>
#include <memory>
#include <deque>
#include <span>
#include <cstdint>
#include <algorithm>
#include <ranges>
#include "errlog.h"
#include "request.h"
#include "printer.h"
#include "bandplane.h"

#ifndef DISABLE_JBIG

/*
 * Fonction de rappel
 * Callback
 */
void Algo0x13::_callback(unsigned char *data, size_t len, void *arg)
{
    auto* info = static_cast<info_t *>(arg);

    if (!len)
        return;

    // It's the first BIH
    if (info->list->empty()) {
        std::vector<uint8_t> bih(data, data + len);
        auto plane = std::make_unique<BandPlane>();
        plane->setData(std::move(bih));
        plane->setEndian(BandPlane::Endian::BigEndian);
        plane->setCompression(0x13);
        info->list->push_back(std::move(plane));
        if (len != 20)
            ERRORMSG(_("the first BIH *MUST* be 20 bytes long (currently={})"), len);
    } else {
        while (len > 0) {
            // Full band: register it
            if (!info->currentData.empty() && info->currentData.size() == info->maxSize) {
                auto plane = std::make_unique<BandPlane>();
                plane->setData(std::move(info->currentData));
                plane->setEndian(BandPlane::Endian::BigEndian);
                plane->setCompression(0x13);
                info->list->push_back(std::move(plane));
                info->currentData.clear();
            }

            // Collect data
            uint32_t freeSpace = info->maxSize - static_cast<uint32_t>(info->currentData.size());
            uint32_t toCopy = std::min(freeSpace, static_cast<uint32_t>(len));
            
            size_t oldSize = info->currentData.size();
            info->currentData.resize(oldSize + toCopy);
            std::ranges::copy(std::span(data, toCopy), info->currentData.begin() + oldSize);
            
            data += toCopy;
            len -= toCopy;
        }
    }
}

// LIFECYCLE: Managed by compiler defaults in the header.
#endif /* DISABLE_JBIG */

#ifndef DISABLE_JBIG
/*
 * Routine de compression
 * Compression routine
 */
SP::Result<std::unique_ptr<BandPlane>> Algo0x13::compress(const Request& request, std::span<const uint8_t> data, 
        uint32_t width, uint32_t height)
{
    jbg85_enc_state state;
    uint32_t i, wbytes;
    info_t info = {&_list, {}, 0};

    if (width > 32768 || height > 32768 || width == 0 || height == 0) {
        ERRORMSG(_("Invalid raster dimensions for Algo0x13: %ux%u"), width, height);
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    if (data.empty()) {
        ERRORMSG(_("Invalid given data for compression (0x13)"));
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    // Compress if it's the first time
    if (!_compressed) {
        if (!request.printer()) {
            return SP::Unexpected(SP::Error::InvalidArgument);
        }

        info.maxSize = static_cast<uint32_t>(request.printer()->packetSize());
        if (!info.maxSize) {
            ERRORMSG(_("PacketSize is set to 0!"));
            info.maxSize = 512*1024;
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

        jbg85_enc_init(&state, width, height, _callback, &info);
        jbg85_enc_options(&state, JBG_LRLTWO | JBG_TPBON, height, 0);

        for (uint32_t i = 0; i < height; i++) {
            const size_t rowOffset = static_cast<size_t>(i) * wbytes;
            const uint8_t* curr = &data[rowOffset];
            const uint8_t* prev = (i > 0) ? &data[rowOffset - wbytes] : nullptr;
            const uint8_t* prev2 = (i > 1) ? &data[rowOffset - (2 * wbytes)] : nullptr;

            jbg85_enc_lineout(&state,
                               const_cast<unsigned char*>(curr),
                               const_cast<unsigned char*>(prev),
                               const_cast<unsigned char*>(prev2));
        }

        // Register the last band
        if (!info.currentData.empty()) {
            auto plane = std::make_unique<BandPlane>();
            plane->setData(std::move(info.currentData));
            plane->setEndian(BandPlane::Endian::BigEndian);
            plane->setCompression(0x13);
            _list.push_back(std::move(plane));
        }
        _compressed = true;
    }

    if (_list.empty()) {
        ERRORMSG(_("Algo0x13 list empty after compression"));
        return SP::Unexpected(SP::Error::LogicError);
    }

    auto plane = std::move(_list.front());
    _list.pop_front();
    return plane;
}

#endif /* DISABLE_JBIG */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */
