/*
 * 	    algo0x13.h                (C) 2007-2008, Aurélien Croc (AP²C)
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
#ifndef _ALGO0X13_H_
#define _ALGO0X13_H_

#ifndef DISABLE_JBIG

#include "algorithm.h"
#include <deque>
#include <memory>
#include <vector>
#include <span>
#include <cstdint>

extern "C" {
#   include "jbig85.h"
}

/**
  * @brief This class implements the compression algorithm 0x13.
  */
class Algo0x13 : public Algorithm
{
    protected:
        struct info_t {
            std::deque<std::unique_ptr<BandPlane>>* list;
            std::vector<uint8_t> currentData;
            uint32_t maxSize;
        };

    protected:
        bool                                    _compressed = false;
        std::deque<std::unique_ptr<BandPlane>>  _list;

    public:
        Algo0x13() = default;
        virtual ~Algo0x13() = default;

    public:
        static void             _callback(unsigned char *data, size_t len, void *arg);

    public:
        virtual SP::Result<std::unique_ptr<BandPlane>> compress(const Request& request, 
                                     std::span<const uint8_t> data, uint32_t width,
                                     uint32_t height) override;
        virtual bool            reverseLineColumn() const override {return false;}
        virtual bool            inverseByte() const override {return false;}
        virtual bool            splitIntoBands() const override {return false;}
};

#endif /* DISABLE_JBIG */

#endif /* _ALGO0X13_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

