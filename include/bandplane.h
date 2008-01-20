/*
 * 	    bandplane.h               (C) 2006-2007, Aurélien Croc (AP²C)
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
#ifndef _BANDPLANE_H_
#define _BANDPLANE_H_

/**
  * @brief This class contains data related to a band plane.
  *
  */
class BandPlane
{
    protected:
        unsigned char           _colorNr;
        unsigned long           _size;
        unsigned char*          _data;

    public:
        /**
          * Initialize the band plane instance.
          */
        BandPlane();
        /**
          * Destroy the instance
          */
        virtual ~BandPlane();

    public:
        /**
          * Set the color number of this plane.
          * @param nr the color number
          */
        void                    setColorNr(unsigned char nr) {_colorNr = nr;}
        /**
          * Set the data size.
          * @param size the data size
          */
        void                    setDataSize(unsigned long size) {_size = size;}
        /**
          * Set the data buffer.
          * The buffer will be freed during the destruction of this instance.
          * @param data the data
          */
        void                    setData(unsigned char *data) {_data = data;}

        /**
          * @return the color number.
          */
        unsigned char           colorNr() const {return _colorNr;}
        /**
          * @return the data size.
          */
        unsigned long           dataSize() const {return _size;}
        /**
          * @return the data.
          */
        const unsigned char*    data() const {return _data;}
};

#endif /* _BANDPLANE_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

