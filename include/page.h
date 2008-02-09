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
 *  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  $Id$
 * 
 */
#ifndef _PAGE_H_
#define _PAGE_H_

#include <stddef.h>

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
        unsigned long           _xResolution;
        unsigned long           _yResolution;
        unsigned long           _width;
        unsigned long           _height;
        unsigned char           _colors;
        unsigned long           _pageNr;
        unsigned long           _copiesNr;
        unsigned long           _compression;
        unsigned char*          _planes[4];
        bool                    _empty;
        unsigned long           _bandsNr;
        Band*                   _firstBand;
        Band*                   _lastBand;

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
        long double             convertToXResolution(long double f)
                                    {return f * _xResolution / 72.;}
        /**
          * Convert a length (given for 72DPI) in the Y resolution.
          * @param f the float to convert.
          * @return the converted value.
          */
        long double             convertToYResolution(long double f)
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
        void                    setXResolution(unsigned long xRes) 
                                    {_xResolution = xRes;}
        /**
          * Set the Y resolution.
          * @param xRes the Y resolution
          */
        void                    setYResolution(unsigned long yRes)
                                    {_yResolution = yRes;}
        /**
          * Set the page width.
          * @param width the page width
          */
        void                    setWidth(unsigned long width)
                                    {_width = width;}
        /**
          * Set the page height.
          * @param height the page height
          */
        void                    setHeight(unsigned long height)
                                    {_height = height;}
        /**
          * Set the number of colors.
          * @param nr the number of colors
          */
        void                    setColorsNr(unsigned char nr) {_colors = nr;}
        /**
          * Set this page number.
          * @param nr this page number
          */
        void                    setPageNr(unsigned long nr) {_pageNr = nr;}
        /**
          * Set the number of copies needed.
          * @param nr the number of copies to do
          */
        void                    setCopiesNr(unsigned long nr)
                                    {_copiesNr = nr;}
        /**
          * Set the compression algorithm number to use.
          * @param nr this compression algorithm number
          */
        void                    setCompression(unsigned long nr)
                                    {_compression = nr;}
        /**
          * Register a new color plane.
          * @param color the color number
          * @param buffer the plane buffer.
          */
        void                    setPlaneBuffer(unsigned char color,
                                    unsigned char* buffer) 
                                    {_planes[color] = buffer; _empty = false;}
        /**
          * Register a new band.
          * Note that band instances will be destroyed when this instance will
          * be destroyed.
          * @param band the band instance.
          */ 
        void                    registerBand(Band *band);
        /**
         * Set this page empty.
         * This is useful in case of compression error.
         */
        void                    setEmpty() {_empty = true;}

        /**
          * @return the X resolution.
          */
        unsigned long           xResolution() const {return _xResolution;}
        /**
          * @return the Y resolution.
          */
        unsigned long           yResolution() const {return _yResolution;}
        /**
          * @return the page width.
          */
        unsigned long           width() const {return _width;}
        /**
          * @return the page height.
          */
        unsigned long           height() const {return _height;}
        /**
          * @return the number of colors.
          */
        unsigned char           colorsNr() const {return _colors;}
        /**
          * @return this page number.
          */
        unsigned long           pageNr() const {return _pageNr;}
        /**
          * @return the number of copies to do.
          */
        unsigned long           copiesNr() const {return _copiesNr;}
        /**
          * @return the compression algorithm number.
          */
        unsigned long           compression() const {return _compression;}
        /**
          * Get the buffer associated to a plane.
          * @param color the color plane number.
          * @return the plane buffer. Otherwise it returns NULL if the color
          *         plane number is incorrect or if there is no plane 
          *         associated.
          */ 
        unsigned char*          planeBuffer(unsigned char color) const
                                    {return color < _colors ? _planes[color] :
                                        NULL;}
        /**
          * @return TRUE if no planes has been set. Otherwise it returns FALSE.
          */ 
        bool                    isEmpty() const {return _empty;}
        /**
          * @return the first band or NULL if no bands has been registered.
          */ 
        const Band*             firstBand() const {return _firstBand;}

    public:
        /**
          * Swap this instance on the disk.
          * @param fd the file descriptor where the instance has to be swapped
          * @return TRUE if the instance has been successfully swapped. 
          *         Otherwise it returns FALSE.
          */
        bool                    swapToDisk(int fd);
        /**
          * Restore an instance from the disk into memory.
          * @param fd the file descriptor where the instance has been swapped
          * @return a page instance if it has been successfully restored. 
          *         Otherwise it returns NULL.
          */
        static Page*            restoreIntoMemory(int fd);
};
#endif /* _PAGE_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

