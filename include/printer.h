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
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  $Id$
 * 
 */
#ifndef _PRINTER_H_
#define _PRINTER_H_

#include <string>
#include <string_view>

class Request;

/**
  * @brief This class contains all the needed information about the printer.
  *
  * This class is mainly used by the QPDL render.
  */
class Printer 
{
    protected:
        std::string             _manufacturer;
        std::string             _model;
        std::string             _beginPJL;
        std::string             _endPJL;

        bool                    _color = false;
        unsigned long           _qpdlVersion = 0;
        unsigned long           _bandHeight = 0;
        bool                    _specialBandWidth = false;
        bool                    _fixedBandWidth = false;
        unsigned long           _packetSize = 0;

        unsigned char           _paperType = 0;
        unsigned char           _paperSource = 0;

        float                   _paperWidth = 0.0f;
        float                   _paperHeight = 0.0f;
        
        unsigned char           _unknownByte1 = 0;
        unsigned char           _unknownByte2 = 0;
        unsigned char           _unknownByte3 = 0;

        float                   _pageWidth = 0.0f;
        float                   _pageHeight = 0.0f;

        float                   _hardMarginX = 0.0f;
        float                   _hardMarginY = 0.0f;

    public:
        /**
          * Initialize a new instance.
          */
        Printer();
        /**
          * Destroy the instance and free the internal memory used.
          */
        virtual ~Printer();

        // Modern Move and Copy operations (Rule of Five)
        Printer(const Printer&) = default;
        Printer& operator=(const Printer&) = default;
        Printer(Printer&&) noexcept = default;
        Printer& operator=(Printer&&) noexcept = default;

    public:
        /**
          * Load the printer configuration requested.
          * @return TRUE if the information have been successfully loaded.
          *         Otherwise it returns FALSE.
          */
        bool                    loadInformation(const Request& request);

        /**
          * Send the PJL header.
          * The PJL header will be sent to STDOUT. Like the other methods which
          * send data to the printer, if you need to redirect the data to
          * somewhere else, use the freopen function to redirect STDOUT.
          * @return TRUE if it succeed. Otherwise it returns FALSE.
          */
        bool                    sendPJLHeader(const Request& request,
                                          unsigned long    compression,
                                          unsigned long    xResolution,
                                          unsigned long    yResolution ) const;
        /**
          * Send the PJL footer.
          * The PJL footer will be sent to STDOUT.
          * @return TRUE if it succeed. Otherwise it returns FALSE.
          */
        bool                    sendPJLFooter(const Request& request) const;

    public:
        /**
          * @return the manufacturer name.
          */
        std::string_view        manufacturer() const {return _manufacturer;}
        /**
          * @return the model name.
          */
        std::string_view        model() const {return _model;}
        /**
          * @return the height of a band.
          */ 
        unsigned long           bandHeight() const {return _bandHeight;}
        /**
         * @return TRUE if the printer requires special BandWidth selection
         *         code (Samsung M2020 Series). Otherwise it returns FALSE.
         */
        bool                    specialBandWidth() const {return _specialBandWidth;}
        /**
         * @return TRUE if the printer requires non-variable BandWidth selection
         *         code (Samsung ML-1510/1520, ML-1610). Otherwise it returns FALSE.
         */
        bool                    fixedBandWidth() const {return _fixedBandWidth;}
        /**
          * @return the maximum size of a packet.
          */ 
        unsigned long           packetSize() const {return _packetSize;}
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
         * @return the page width.
         */
        float                   pageWidth() const {return _pageWidth;}
        /**
         * @return the page height.
         */
        float                   pageHeight() const {return _pageHeight;}
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
        /**
         * @return the hard margin of the printer in the X direction.
         */
        float                   hardMarginX() const {return _hardMarginX;}
        /**
         * @return the hard margin of the printer in the Y direction.
         */
        float                   hardMarginY() const {return _hardMarginY;}

};

#endif /* _PRINTER_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

