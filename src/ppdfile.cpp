/*
 * 	    ppdfile.cpp               (C) 2006-2008, Aurélien Croc (AP²C)
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
#include "ppdfile.h"
#include <cups/cups.h>
#include <string.h>
#include <ctype.h>
#include "errlog.h"



/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
PPDFile::PPDFile()
{
    _ppd = NULL;
}

PPDFile::~PPDFile()
{
    close();
}



/*
 * Ouverture et fermeture du fichier PPD
 * Opening or closing PPD file
 */
bool PPDFile::open(const char *file, const char *version, const char *useropts)
{
    const char *fileVersion;
    cups_option_t *options;
    int nr;

    // Check if a PPD file was previously opened
    if (_ppd) {
        ERRORMSG(_("Internal warning: a PPD file was previously opened. Please "
            "close it before opening a new one."));
        close();
    }

    // Open the PPD file
    _ppd = ppdOpenFile(file);
    if (!_ppd) {
        ERRORMSG(_("Cannot open PPD file %s"), file);
        return false;
    }

    // Mark the default values and the user options
    ppdMarkDefaults(_ppd);
    nr = cupsParseOptions(useropts, 0, &options);
    cupsMarkOptions(_ppd, nr, options);
    cupsFreeOptions(nr, options);

    // Check if the PPD version is compatible with this filter
    fileVersion = get("FileVersion");
    if (!fileVersion) { 
        ERRORMSG(_("No FileVersion found in the PPD file: invalid "
            "PPD file"));
        ppdClose(_ppd);
        _ppd = NULL;
        return false;
    }
    if (strcmp(fileVersion, version)) {
        ERRORMSG(_("Invalid PPD file version: %s but the PPD file "
            "is designed for SpliX V. %s"), version, fileVersion);
        ppdClose(_ppd);
        _ppd = NULL;
        return false;
    }

    return true;
}

void PPDFile::close()
{
    if (!_ppd)
        return;
    ppdClose(_ppd);
    _ppd = NULL;
}




/*
 * Obtention d'une donnée du fichier PPD
 * Get a string from the PPD file
 */
PPDValue PPDFile::get(const char *name, const char *opt)
{
    ppd_attr_t *attr;
    PPDValue val;

    if (!_ppd) {
        ERRORMSG(_("Trying to read a PPD file which wasn't opened"));
        return val;
    }

    if (!opt)
        attr = ppdFindAttr(_ppd, name, NULL);
    else
        attr = ppdFindAttr(_ppd, opt, name);
    if (!attr) {
        ppd_choice_t *choice;
        choice = ppdFindMarkedChoice(_ppd, name);
        if (!choice)
            return val;
        val.set(choice->choice);
    } else
        val.set(attr->value);
    return val;
}

PPDValue PPDFile::getPageSize(const char *name)
{
    ppd_size_t *s;
    PPDValue val;

    if (!(s = ppdPageSize(_ppd, name)))
        return val;
    val.set(s->width, s->length, s->left, s->bottom);
    return val;
}



/*
 * Gestion des valeurs du PPD
 * PPD values management
 */
PPDFile::Value::Value()
{
    _value = NULL;
    _out = NULL;
    _preformatted = NULL;
    _width = 0.;
    _height = 0.;
    _marginX = 0.;
    _marginY = 0.;
}

PPDFile::Value::Value(const char *value)
{
    _value = value;
    _out = value;
    _preformatted = NULL;
    _width = 0.;
    _height = 0.;
    _marginX = 0.;
    _marginY = 0.;
}

PPDFile::Value::~Value()
{
    if (_preformatted)
        delete[] _preformatted;
}

PPDFile::Value& PPDFile::Value::set(const char *value)
{
    if (_preformatted) {
        delete[] _preformatted;
        _preformatted = NULL;
    }
    _value = value;
    _out = _value;

    return *this;
}

PPDFile::Value& PPDFile::Value::set(float width, float height, float marginX,
    float marginY)
{
    _width = width;
    _height = height;
    _marginX = marginX;
    _marginY = marginY;

    return *this;
}

PPDFile::Value& PPDFile::Value::setPreformatted()
{
    const char *str = _value;
    unsigned int i;

    if (!str)
        return *this;

    _preformatted = new char[strlen(str)];
    for (i=0; *str; str++) {
        if (*str == '<' && strlen(str) >= 3 && isxdigit(*(str+1))) {
            char temp[3] = {0, 0, 0};

            str++;
            temp[0] = *str;
            str++;
            if (*str != '>' && (!isxdigit(*str) || *(str + 1) != '>')) {
                _preformatted[i] = '<';
                _preformatted[i+1] = temp[0];
                i += 2;
                continue;
            }
            if (*str != '>') {
                temp[1] = *str;
                str++;
            }
            _preformatted[i] = (char)strtol((char *)&temp, (char **)NULL,16);
            i++;
            continue;
        }
        _preformatted[i] = *str;
        i++;
    }
    _preformatted[i] = 0;
    _out = _preformatted;

    return *this;
}

char* PPDFile::Value::deepCopy() const
{
    char *tmp;

    if (!_out)
        return NULL;
    tmp = new char[strlen(_out)+1];
    strcpy(tmp, _out);

    return tmp;
}

bool PPDFile::Value::isTrue() const
{
    if (!_out)
        return false;
    if (!strcmp(_out, "1") || !strcasecmp(_out, "true") || 
        !strcasecmp(_out, "enable") || !strcasecmp(_out, "enabled") || 
        !strcasecmp(_out, "yes") || !strcasecmp(_out, "on"))
        return true;
    return false;
}

bool PPDFile::Value::operator == (const char* val) const
{
    if (!_out)
        return false;
    return !strcasecmp(_out, val);
}

bool PPDFile::Value::operator != (const char* val) const
{
    if (!_out)
        return true;
    return strcasecmp(_out, val);
}



/*
 * Opérateur d'assignation
 * Assignment operator
 */
void PPDFile::Value::operator = (const PPDFile::Value::Value &val)
{
    if (_preformatted)
        delete[] _preformatted;
    _value = val._value;
    _out = val._out;
    _preformatted = val._preformatted;
    _width = val._width;
    _height = val._height;
    _marginX = val._marginX;
    _marginY = val._marginY;
}


/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

