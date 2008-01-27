/*
 * 	    printer.h                 (C) 2006-2008, Aurélien Croc (AP²C)
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
#ifndef _PRINTER_H_
#define _PRINTER_H_

class Request;

/**
  * @brief This class contains all the needed information about the printer.
  *
  * This class is mainly used by the QPDL render.
  */
class Printer 
{
    protected:
        char*                   _manufacturer;
        char*                   _model;

        bool                    _color;
        unsigned long           _qpdlVersion;
        unsigned long           _bandHeight;

        unsigned char           _paperType;
        unsigned char           _paperSource;
        
        unsigned char           _unknownByte1;
        unsigned char           _unknownByte2;
        unsigned char           _unknownByte3;

    public:
        /**
          * Initialize a new instance.
          */
        Printer();
        /**
          * Destroy the instance and free the internal memory used.
          */
        virtual ~Printer();

    public:
        /**
          * Load the printer configuration requested.
          * @return TRUE if the information have been successfully loaded.
          *         Otherwise it returns FALSE.
          */
        bool                    loadInformation(const Request& request);

    public:
        /**
          * @return the manufacturer name.
          */
        const char*             manufacturer() const {return _manufacturer;}
        /**
          * @return the model name.
          */
        const char*             model() const {return _model;}
        /**
          * @return the height of a band.
          */ 
        unsigned long           bandHeight() const {return _bandHeight;}
        /**
          * @return the QPDL version.
          */
        unsigned long           qpdlVersion() const {return _qpdlVersion;}
        /**
         * @return TRUE if this printer is a color printer. Otherwise it returns
         *         FALSE.
         */
        bool                    color() const {return _color;}
        /**
         * @return the paper source.
         */
        unsigned char           paperSource() const {return _paperSource;}
        /**
         * @return the paper type.
         */
        unsigned char           paperType() const {return _paperType;}
        /**
         * @return the unknown byte 1
         */
        unsigned char           unknownByte1() const {return _unknownByte1;}
        /**
         * @return the unknown byte 2
         */
        unsigned char           unknownByte2() const {return _unknownByte2;}
        /**
         * @return the unknown byte 3
         */
        unsigned char           unknownByte3() const {return _unknownByte3;}
};

#endif /* _PRINTER_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

