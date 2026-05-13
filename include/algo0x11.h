/*
 *         algo0x11.h                (C) 2006-2008, Aurélien Croc (AP²C)
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
 */
#ifndef _ALGO0X11_H_
#define _ALGO0X11_H_

#include <vector>
#include <memory>
#include <span>
#include <cstdint>
#include "algorithm.h"
#include "sp_result.h"

/**
  * @brief This class implements the compression algorithm 0x11.
  */
class Algo0x11 : public Algorithm
{
    public:
        static constexpr uint32_t COMPRESS_SAMPLE_RATE   = 0x800;
        static constexpr uint32_t TABLE_PTR_SIZE         = 0x40;
        static constexpr uint32_t MAX_UNCOMPRESSED_BYTES = 0x80;
        static constexpr uint32_t MAX_COMPRESSED_BYTES   = 0x202;
        static constexpr uint32_t MIN_COMPRESSED_BYTES   = 0x2;
        static constexpr uint8_t  COMPRESSION_FLAG       = 0x80;

    protected:
        uint32_t                _ptrArray[TABLE_PTR_SIZE];

    protected:
        SP::Result<>            _lookupBestOccurs(std::span<const uint8_t> data);
        SP::Result<>            _compress(std::span<const uint8_t> data, 
                                    std::vector<uint8_t> &output);

    public:
        Algo0x11() = default;
        virtual ~Algo0x11() = default;

    public:
        virtual SP::Result<std::unique_ptr<BandPlane>> compress(const Request& request, 
                                     std::span<const uint8_t> data, uint32_t width,
                                     uint32_t height) override;
        virtual bool            reverseLineColumn() const override {return true;}
        virtual bool            inverseByte() const override {return true;}
        virtual bool            splitIntoBands() const override {return true;}
};

#endif /* _ALGO0X11_H_ */
