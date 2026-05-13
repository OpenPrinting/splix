/*
 *         algo0x0d.h SpliX is Copyright 2006-2008 by Aurélien Croc
 *                    This file is a SpliX derivative work
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
 * $Id$
 * --
 *  This code is written by Leonardo Hamada
 */
#ifndef _ALGO0x0D_H_
#define _ALGO0x0D_H_

#include "algorithm.h"
#include <vector>
#include <cstdint>


/**
  * @brief This class implements the type 0xd encoding.
  */
class Algo0x0D : public Algorithm
{
    protected:
        inline void writeTwoBytesPacket(
                    std::vector<uint8_t> &output,
                    int32_t             accumulatedHorizontalOffsetValue,
                    int32_t             accumulatedVerticalOffsetValue,
                    uint32_t            accumulatedRunCount );
    
        inline void writeFourBytesPacket(
                    std::vector<uint8_t> &output,
                    int32_t             accumulatedHorizontalOffsetValue,
                    int32_t             accumulatedVerticalOffsetValue,
                    uint32_t            accumulatedRunCount );
    
        inline void writeSixBytesPacket(
                    std::vector<uint8_t> &output,
                    uint32_t            accumulatedCombinedOffsetValue,
                    uint32_t            accumulatedRunCount );
    
        inline void selectPacketSize(
                    std::vector<uint8_t> &output,
                    uint32_t            preAccumulatedHorizontalOffsetValue,
                    uint32_t            accumulatedHorizontalOffsetValue,
                    uint32_t            currentHorizontalPenPosition,
                    uint32_t            accumulatedRunCount,
                    uint32_t            consecutiveBlankScanLines,
                    uint32_t            currentVerticalPenPosition,
                    const uint32_t      wrapWidth );

    public:
        Algo0x0D() = default;
        virtual ~Algo0x0D() = default;

    public:
        virtual SP::Result<std::unique_ptr<BandPlane>> compress(const Request& request, 
                                    std::span<const uint8_t> data, uint32_t width,
                                    uint32_t height) override;
        virtual bool            reverseLineColumn() const override {return false;}
        virtual bool            inverseByte() const override {return false;}
        virtual bool            splitIntoBands() const override {return true;}
};

#endif /* _ALGO0x0D_H_ */
