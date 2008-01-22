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
 *  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  $Id$
 * 
 */
#ifndef _PPDFILE_H_
#define _PPDFILE_H_

#include <cups/ppd.h>

/**
  * @class PPDFile
  * @brief Manage easily PPD files.
  *
  * This class provides methods to access in a easy way to data contained in PPD
  * files. During the opening of the file, the PPD version can be compared with
  * the current project version, default and user values are set as default.
  * Next, data contained in the ppd file can be manipulated easily with two
  * methods. The second one (@ref PPDValue::setPreformatted) allows the 
  * use of special bytes in strings.
  */
class PPDFile
{
    public:
        /**
          * @brief This class manages a PPD value.
          *
          * Use the defined type PPDValue to use this class.
          *
          * In a PPD a string can be preformatted to contain unprintable 
          * characters. For that, insert the ASCII number between < and >.
          * @code
          * *QPDL beginPJL: "<1B>%-12345X"
          * @endcode
          *
          * To decode these preformatted string, call the @ref setPreformatted
          * method and read the string.
          *
          */
        class Value {
            protected:
                const char*         _value;
                char*               _preformatted;
                const char*         _out;

            public:
                /**
                 * Initialize a new instance.
                 */
                Value();
                /**
                 * Initialize a new instance with the specified string.
                 * @param value the specified value.
                 */
                Value(const char *value);
                /**
                 * Destroy the instance.
                 */
                virtual ~Value();

            public:
                /**
                 * Set a string.
                 * @param value the string value.
                 * @return itself.
                 */
                PPDFile::Value&     set(const char *value);
                /**
                 * Specify the represented string is preformatted.
                 * @return itself.
                 */
                PPDFile::Value&     setPreformatted();

            public:
                /**
                  * @return TRUE if there is no associated string. Otherwise it
                  *         returns FALSE.
                  */
                bool                isNull() const {return _out ? false : true;}

                /**
                  * Copy the string into an allocated buffer.
                  * The user has to free the string at the end of its use.
                  * @return a pointer to an allocated buffer containing the
                  *         string. If there is no string, it returns NULL.
                  */
                char*               deepCopy() const;

                /**
                  * @return TRUE if the key is set to true, enable, enabled,
                  *         yes, 1. Otherwise it returns FALSE.
                  */
                bool                isTrue() const;
                /**
                  * @return FALSE if the key is set to true, enable, enabled,
                  *         yes, 1. Otherwise it returns TRUE.
                  */
                bool                isFalse() const {return !isTrue();}

                /**
                 * @return the string pointer.
                 */
                operator const char*() const {return _out;}
                /**
                 * @return the unsigned long converted value.
                 */
                operator unsigned long() const 
                        {return _out ? strtol(_out, (char**)NULL, 10) : 0;}
                /**
                 * @return the long converted value.
                 */
                operator long() const 
                        {return _out ? strtol(_out, (char**)NULL, 10) : 0;}
                /**
                 * @return the float converted value.
                 */
                operator float() const 
                        {return _out ? strtof(_out, (char**)NULL) : 0;}
                /**
                 * @return the double converted value.
                 */
                operator double() const 
                        {return _out ? strtod(_out, (char**)NULL) : 0;}
                /**
                 * @return the long double converted value.
                 */
                operator long double() const 
                        {return _out ? strtold(_out, (char**)NULL) : 0;}
        };

    protected:
        ppd_file_t*             _ppd;

    public:
        /**
         * Initialize a new PPDFile instance.
         */
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
          * @return TRUE if the PPD has been successfully opened. Otherwise it
          *         returns false.
          */
        bool                    open(const char *file, const char *version, 
                                    const char *useropts = "");
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
        Value                     get(const char *name, const char *opt=NULL);
};

/**
 * Represent a PPD value
 */
typedef PPDFile::Value PPDValue;


#endif /* _PPDFILE_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

