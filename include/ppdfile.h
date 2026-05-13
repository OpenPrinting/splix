/*
 * 	    ppdfile.h                 (C) 2006-2008, Aurélien Croc (AP²C)
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
#ifndef _PPDFILE_H_
#define _PPDFILE_H_

#include <cups/cups.h>
#include <cups/ppd.h>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <charconv>
#include "sp_result.h"

/**
  * @class PPDFile
  * @brief This class provides an easy method to manage printer capabilities.
  *
  * This class provides methods to access printer capabilities and job options
  * using the modern CUPS Destination Information API.
  */
class PPDFile
{
    public:
        /**
          * @brief This class manages a PPD value.
          *
          * Use the defined type PPDValue to use this class.
          */
        class Value {
            protected:
                std::string         _raw;
                std::string         _preformatted;
                float               _width;
                float               _height;
                float               _marginX;
                float               _marginY;

            public:
                Value();
                Value(std::string_view value);
                virtual ~Value();

            public:
                Value&              set(std::string_view value);
                Value&              set(float width, float height, float
                                        marginX, float marginY);
                Value&              setPreformatted();

            public:
                float               width() const {return _width;}
                float               height() const {return _height;}
                float               marginX() const {return _marginX;}
                float               marginY() const {return _marginY;}
                bool                isNull() const {return _out.empty() && !_hasValue;}
                std::string         deepCopy() const;
                bool                isTrue() const;
                bool                isFalse() const {return !isTrue();}
                operator bool() const {return !isNull();}
                operator const char*() const {return isNull() ? nullptr : _out.c_str();}
                operator unsigned long() const;
                operator long() const;
                operator float() const;
                operator double() const;
                operator long double() const;
                bool    operator == (std::string_view val) const;
                bool    operator == (const char *val) const;
                bool    operator != (std::string_view val) const;
                bool    operator != (const char *val) const;
            private:
                std::string         _out;
                bool                _hasValue = false;
            public:
                Value(const Value& other);
                Value&  operator = (const Value &val);
        };

    protected:
        struct DestDeleter { void operator()(cups_dest_t *d) const { if (d) cupsFreeDests(1, d); } };
        struct DInfoDeleter { void operator()(cups_dinfo_t *d) const { if (d) cupsFreeDestInfo(d); } };
        struct PPDDeleter { void operator()(ppd_file_t *p) const { if (p) ppdClose(p); } };

        std::unique_ptr<cups_dest_t, DestDeleter>   _dest;
        std::unique_ptr<cups_dinfo_t, DInfoDeleter> _dinfo;
        std::unique_ptr<ppd_file_t, PPDDeleter>     _ppd;
        int              _num_options;
        cups_option_t   *_options;
        std::string      _ppdPath;

    public:
        PPDFile();
        /**
         * Destroy the instance.
         * the PPD file will be closed if it was opened.
         */
        virtual~ PPDFile();

    public:
        /**
          * Open a PPD file and check its integrity.
          * @param file the file path and name
          * @param version the current SpliX version
          * @param useropts the user options
          * @return SP::Result<> indicating success or error code.
          */
        SP::Result<>            open(std::string_view file, std::string_view version, 
                                    std::string_view useropts = "");
        /**
          * Close a previously opened PPD file.
          */
        void                    close();

    public:
        /**
          * Get the string associated to a key or a key and a group.
          * @param name the key name
          * @param opt the name of the group if the key is in the group.
          *            Otherwise it must be set to NULL
          * @return a PPDValue instance containing the string or NULL if the key
          *         or the group/key doesn't exists or if there is no data 
          *         associated.
          */
        /**
          * Get the string associated to a key or a key and a group.
          * @param name the key name
          * @param opt the name of the group if the key is in the group.
          *            Otherwise it must be set to NULL
          * @return a PPDValue instance containing the string or NULL if the key
          *         or the group/key doesn't exists or if there is no data 
          *         associated.
          */
        Value                   get(std::string_view name, std::string_view opt = "");
        /**
          * Get the page size information.
          * @param name the page format name
          * @return a PPDValue instance containing the width and the height of
          *         the page format requested.
          */
        Value                   getPageSize(std::string_view name);
};

/**
 * Represent a PPD value
 */
typedef PPDFile::Value PPDValue;


#endif /* _PPDFILE_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

