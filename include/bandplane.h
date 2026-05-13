/*
 * 	    bandplane.h               (C) 2006-2008, Aurélien Croc (AP²C)
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
#ifndef _BANDPLANE_H_
#define _BANDPLANE_H_

#include <vector>
#include <cstdint>
#include <memory>
#include <span>
#include "sp_result.h"

/**
  * @brief This class contains data related to a band plane.
  *
  */
class BandPlane
{
    public:
        enum class Endian : uint8_t {
            /** Machine dependant */
            Dependant,
            /** Big endian */
            BigEndian,
            /** Little endian */
            LittleEndian,
        };

    protected:
        uint8_t                 _colorNr = 0;
        std::vector<uint8_t>    _data;
        uint32_t                _checksum = 0;
        Endian                  _endian = Endian::Dependant;
        uint8_t                 _compression = 0;

    public:
        /**
          * Initialize the band plane instance.
          */
        BandPlane() = default;
        /**
          * Destroy the instance
          */
        virtual ~BandPlane() = default;

    public:
        /**
          * Set the color number of this plane.
          * @param nr the color number
          */
        void                    setColorNr(uint8_t nr) {_colorNr = nr;}
        /**
          * Set the data buffer.
          * @param data the data buffer
          */
        void                    setData(std::vector<uint8_t> data);
        /**
          * Set the endian to use.
          * @param endian the endian to use.
          */
        void                    setEndian(Endian endian) {_endian = endian;}
        /**
         * Set the compression algorithm used.
         * @param compression the compression algorithm used.
         */
        void                    setCompression(uint8_t compression)
                                    {_compression = compression;}

        /**
          * @return the color number.
          */
        uint8_t                 colorNr() const {return _colorNr;}
        /**
          * @return the data size.
          */
        size_t                  dataSize() const {return _data.size();}
        /**
          * @return the data.
          */
        const uint8_t*          data() const {return _data.data();}
        /**
          * @return a span viewing the buffer data.
          */
        std::span<const uint8_t> data_span() const noexcept { return _data; }
        /**
          * @return the endian to use.
          */
        Endian                  endian() const {return _endian;}
        /**
          * @return the checksum.
          */
        uint32_t                checksum() const {return _checksum;}
        /**
         * @return the compression algorithm used.
         */
        uint8_t                 compression() const {return _compression;}

    public:
        /**
          * Swap this instance on the disk.
          * @param fd the file descriptor where the instance has to be swapped
          * @return a Result indicating success or error.
          */
        SP::Result<>            swapToDisk(int fd);
        /**
          * Restore an instance from the disk into memory.
          * @param fd the file descriptor where the instance has been swapped
          * @return a Result containing the bandplane instance or an error.
          */
        static SP::Result<std::unique_ptr<BandPlane>> restoreIntoMemory(int fd);
};

#endif /* _BANDPLANE_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

