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
 *  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  $Id$
 * 
 */
#ifndef _BAND_H_
#define _BAND_H_

#include <stddef.h>

class Page;
class BandPlane;

/**
  * @brief This class contains all information related to a band.
  *
  */
class Band
{
    protected:
        unsigned long           _bandNr;
        const Page*             _parent;
        unsigned char           _colors;
        BandPlane*              _planes[4];
        unsigned long           _width;
        unsigned long           _height;
        Band*                   _sibling;

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
        Band (unsigned long nr, unsigned long width, unsigned long height);
        /**
          * Destroy the instance
          */
        virtual ~Band();

    public:
        /**
          * Set the band number.
          * @param nr the band number
          */
        void                    setBandNr(unsigned long nr) {_bandNr = nr;}
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
        void                    registerPlane(BandPlane* plane)
                                {if (_colors < 4) {_planes[_colors] = plane;
                                        _colors++;}}
        /**
          * Set the band width.
          * @param width the band width
          */
        void                    setBandWidth(unsigned long width)
                                    {_width = width;}
        /**
          * Set the band height.
          * @param height the band height
          */
        void                    setBandHeight(unsigned long height)
                                    {_height = height;}
        /**
          * Register sibling.
          * @param sibling the sibling.
          */
        void                    registerSibling(Band* sibling) 
                                    {_sibling = sibling;}

        /**
          * @return the band number.
          */
        unsigned long           bandNr() const {return _bandNr;}
        /**
          * @return the number of registered planes.
          */
        unsigned long           planesNr() const {return _colors;}
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
        const BandPlane*        plane(unsigned char nr) const 
                                    {return nr < _colors ? _planes[nr] : NULL;}
        /**
          * @return the band width.
          */
        unsigned long           bandWidth() const {return _width;}
        /**
          * @return the band height.
          */
        unsigned long           bandHeight() const {return _height;}
        /**
          * @return the next sibling or NULL if there is no sibling.
          */
        Band*                   sibling() const {return _sibling;}

};

#endif /* _BAND_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

