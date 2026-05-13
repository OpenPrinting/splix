/*
 *         algo0x0e.h SpliX is Copyright 2006-2008 by Aurélien Croc
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
 * This code has been written by Leonardo Hamada
 */
#ifndef _ALGO0x0E_H_
#define _ALGO0x0E_H_

#include "algorithm.h"
#include <vector>
#include <span>
#include <memory>
#include <cstdint>

/**
  * @brief This class implements the type 0xe encoding.
  */
class Algo0x0E : public Algorithm
{
    protected:    
        inline void addLiteralSequence(
                                        std::vector<uint8_t> &output,
                                        std::span<const uint8_t> data,
                                        uint32_t position,
                                        uint32_t length,
                                        uint32_t blanks );
        inline void addReplicativeRun(
                                        std::vector<uint8_t> &output,
                                        uint32_t runs,
                                        uint8_t value );
        uint32_t verifyGain(std::span<const uint8_t> data);
        uint32_t encodeReplications(uint32_t q,
                                         uint32_t L,
                                         std::span<const uint8_t> data,
                                         std::vector<uint8_t> &output);
        uint32_t locateBackwardReplications(std::span<const uint8_t> data);

    public:
        Algo0x0E() = default;
        virtual ~Algo0x0E() = default;

    public:
        virtual SP::Result<std::unique_ptr<BandPlane>> compress(const Request& request, 
                                    std::span<const uint8_t> data, uint32_t width,
                                    uint32_t height) override;
        virtual bool            reverseLineColumn() const override {return false;}
        virtual bool            inverseByte() const override {return true;}
        virtual bool            splitIntoBands() const override {return true;}
};

#endif /* _ALGO0x0E_H_ */
