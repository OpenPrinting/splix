/*
 * 	    page.h                    (C) 2006-2008, Aurélien Croc (AP²C)
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
#ifndef _PAGE_H_
#define _PAGE_H_

#include <memory>
#include <vector>
#include <array>
#include <cstdint>
#include "sp_result.h"

class Band;

/**
  * @brief This class contains a page representation.
  *
  * There are two steps. During the first step, the instance contains the bitmap
  * representation of the page. During the second step, the bitmap
  * representation is freed and is replaced by bands representations.
  * Each band is linked to this instance and will be used by the QPDL render.
  *
  * When the page will be compressed this instance have to be sent to the cache
  * manager for waiting its use by the render code.
  */
class Page
{
    protected:
        uint32_t                            _xResolution = 0;
        uint32_t                            _yResolution = 0;
        uint32_t                            _width = 0;
        uint32_t                            _height = 0;
        uint8_t                             _colors = 0;
        uint32_t                            _pageNr = 0;
        uint32_t                            _copiesNr = 0;
        uint32_t                            _compression = 0;
        std::array<std::vector<uint8_t>, 4> _planes;
        bool                                _empty = true;
        uint32_t                            _bandsNr = 0;
        std::vector<uint8_t>                _bih;
        std::unique_ptr<Band>               _firstBand;
        Band*                               _lastBand = nullptr; // Non-owning observer

    public:
        /**
          * Initialize the page instance.
          */
        Page();
        /**
          * Destroy the instance
          */
        virtual ~Page();

    public:
        /**
          * Convert a length (given for 72DPI) in the X resolution.
          * @param f the float to convert.
          * @return the converted value.
          */
        long double             convertToXResolution(long double f) const
                                    {return f * _xResolution / 72.;}
        /**
          * Convert a length (given for 72DPI) in the Y resolution.
          * @param f the float to convert.
          * @return the converted value.
          */
        long double             convertToYResolution(long double f) const
                                    {return f * _yResolution / 72.;}

        /**
          * Delete all planes registered and free the used memory.
          * This function has to be called when planes are no longer
          * used to free the huge amount of memory required for them.
          */
        void                    flushPlanes();

        /**
          * Make a 180° rotation of the bitmap planes
          */
        void                    rotate();

    public:
        /**
          * Set the X resolution.
          * @param xRes the X resolution
          */
        void                    setXResolution(uint32_t xRes) 
                                    {_xResolution = xRes;}
        /**
          * Set the Y resolution.
          * @param yRes the Y resolution
          */
        void                    setYResolution(uint32_t yRes)
                                    {_yResolution = yRes;}
        /**
          * Set the page width.
          * @param width the page width
          */
        void                    setWidth(uint32_t width)
                                    {_width = width;}
        /**
          * Set the page height.
          * @param height the page height
          */
        void                    setHeight(uint32_t height)
                                    {_height = height;}
        /**
          * Set the number of colors.
          * @param nr the number of colors
          */
        void                    setColorsNr(uint8_t nr) {_colors = nr;}
        /**
          * Set this page number.
          * @param nr this page number
          */
        void                    setPageNr(uint32_t nr) {_pageNr = nr;}
        /**
          * Set the number of copies needed.
          * @param nr the number of copies to do
          */
        void                    setCopiesNr(uint32_t nr)
                                    {_copiesNr = nr;}
        /**
          * Set the compression algorithm number to use.
          * @param nr this compression algorithm number
          */
        void                    setCompression(uint32_t nr)
                                    {_compression = nr;}
        /**
          * Register a new color plane.
          * @param color the color number
          * @param buffer the plane buffer.
          * @return a Result indicating success or MemoryError.
          */
        SP::Result<>            setPlaneBuffer(uint8_t color,
                                    std::vector<uint8_t> buffer);
        /**
          * Register a new band.
          * Note that band instances will be destroyed when this instance will
          * be destroyed.
          * @param band the band instance.
          */ 
        void                    registerBand(std::unique_ptr<Band> band);
        /**
          * Set this page empty.
          * This is useful in case of compression error.
          */
        void                    setEmpty() {_empty = true;}

        /**
          * @return the X resolution.
          */
        uint32_t                xResolution() const {return _xResolution;}
        /**
          * @return the Y resolution.
          */
        uint32_t                yResolution() const {return _yResolution;}
        /**
          * @return the page width.
          */
        uint32_t                width() const {return _width;}
        /**
          * @return the page height.
          */
        uint32_t                height() const {return _height;}
        /**
          * @return the number of colors.
          */
        uint8_t                 colorsNr() const {return _colors;}
        /**
          * @return the number of registered bands.
          */
        uint32_t                bandsNr() const {return _bandsNr;}
        /**
          * @return this page number.
          */
        uint32_t                pageNr() const {return _pageNr;}
        /**
          * @return the number of copies to do.
          */
        uint32_t                copiesNr() const {return _copiesNr;}
        /**
          * @return the compression algorithm number.
          */
        uint32_t                compression() const {return _compression;}
        /**
          * Get the buffer associated to a plane.
          * @param color the color plane number.
          * @return a pointer to the plane buffer. Otherwise it returns NULL if the color
          *         plane number is incorrect or if there is no plane 
          *         associated.
          */ 
        uint8_t*                planeBuffer(uint8_t color)
                                    {return color < _colors && !_planes[color].empty() 
                                        ? _planes[color].data() : nullptr;}
        /**
          * Get the buffer associated to a plane (const version).
          * @param color the color plane number.
          * @return a pointer to the plane buffer. Otherwise it returns NULL if the color
          *         plane number is incorrect or if there is no plane 
          *         associated.
          */ 
        const uint8_t*          planeBuffer(uint8_t color) const
                                    {return color < _colors && !_planes[color].empty() 
                                        ? _planes[color].data() : nullptr;}
        /**
          * @return TRUE if no planes has been set. Otherwise it returns FALSE.
          */ 
        bool                    isEmpty() const {return _empty;}
        /**
          * @return the first band or NULL if no bands has been registered.
          */ 
        const Band*             firstBand() const {return _firstBand.get();}

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
          * @return a Result containing the page instance or an error.
          */
        static SP::Result<std::unique_ptr<Page>> restoreIntoMemory(int fd);
        /**
          * Register an independent copy of the BIH data. 
          * @param bih_data the BIH for JBIG data.
          * @param size the BIH size (default 20)
          */
        void                    setBIH(const uint8_t *bih_data, size_t size = 20);
        /**
          * Returns the BIH data belonging to the Page object. 
          */
        const uint8_t*          getBIH() const { return _bih.empty() ? nullptr : _bih.data(); }
};
#endif /* _PAGE_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

