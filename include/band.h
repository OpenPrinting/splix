/*
 * 	    band.h                    (C) 2006-2008, Aurélien Croc (AP²C)
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
#ifndef _BAND_H_
#define _BAND_H_

#include <vector>
#include <memory>
#include <ranges>
#include <cstdint>
#include "sp_result.h"

class Page;
class BandPlane;

/**
  * @brief This class contains all information related to a band.
  *
  */
class Band
{
    protected:
        uint32_t                                _bandNr = 0;
        const Page*                             _parent = nullptr;
        std::vector<std::unique_ptr<BandPlane>> _planes;
        uint32_t                                _width = 0;
        uint32_t                                _height = 0;
        std::unique_ptr<Band>                   _sibling = nullptr;

    public:
        /**
          * Initialize the band instance.
          */
        Band();
        /**
          * Initialize the band instance.
          * @param nr the band number
          * @param width the band width
          * @param height the band height
          */
        Band (uint32_t nr, uint32_t width, uint32_t height);
        /**
          * Destroy the instance
          */
        virtual ~Band();

    public:
        /**
          * Set the band number.
          * @param nr the band number
          */
        void                    setBandNr(uint32_t nr) {_bandNr = nr;}
        /**
          * Register the parent @ref Page instance.
          * @param parent the parent page instance
          */
        void                    registerParent(const Page* parent) 
                                {_parent = parent;}
        /**
          * Register a new plane.
          * @param plane the band plane
          */
        void                    registerPlane(std::unique_ptr<BandPlane> plane);
        /**
          * Set the band width.
          * @param width the band width
          */
        void                    setWidth(uint32_t width)
                                    {_width = width;}
        /**
          * Set the band height.
          * @param height the band height
          */
        void                    setHeight(uint32_t height)
                                    {_height = height;}
        /**
          * Register sibling.
          * @param sibling the sibling.
          */
        void                    registerSibling(std::unique_ptr<Band> sibling) 
                                    {_sibling = std::move(sibling);}

        /**
          * @return the band number.
          */
        uint32_t                bandNr() const {return _bandNr;}
        /**
          * @return the number of registered planes.
          */
        size_t                  planesNr() const {return _planes.size();}
        /**
          * @return the parent @ref Page instance.
          */
        const Page*             parent() const {return _parent;}
        /**
          * Get a specific band plane.
          * @param nr the band plane number
          * @return the @ref BandPlane instance if it exists. Otherwise it
          *         returns NULL.
          */
        const BandPlane*        plane(size_t nr) const 
                                    {return nr < _planes.size() ? _planes[nr].get() : nullptr;}
        /**
          * @return a view of the planes.
          */
        auto                    planes() const {
             return _planes | std::views::transform([](const auto& p) { return p.get(); });
        }
        /**
          * @return the band width.
          */
        uint32_t                width() const {return _width;}
        /**
          * @return the band height.
          */
        uint32_t                height() const {return _height;}
        /**
          * @return the next sibling or NULL if there is no sibling.
          */
        Band*                   sibling() const {return _sibling.get();}

    public:
        /**
          * Swap this instance on the disk.
          * @param fd the file descriptor where the instance has to be swapped
          * @return a Result indicating success or the specific error.
          */
        SP::Result<>            swapToDisk(int fd);
        /**
          * Restore an instance from the disk into memory.
          * @param fd the file descriptor where the instance has been swapped
          * @return a Result containing the band instance or an error.
          */
        static SP::Result<std::unique_ptr<Band>> restoreIntoMemory(int fd);
};

#endif /* _BAND_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */
