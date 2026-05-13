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
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  $Id$
 * 
 */
#include "ppdfile.h"
#include <cups/ppd.h>
#include <cups/cups.h>
#include <cstring>
#include <ctype.h>
#include <cstdio>
#include <algorithm>
#include "errlog.h"
#include "sp_portable.h"

/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
PPDFile::PPDFile() : 
    _num_options(0), _options(nullptr)
{
}

PPDFile::~PPDFile() = default;

/*
 * Ouverture et fermeture du fichier PPD
 * Opening or closing PPD file
 */
SP::Result<> PPDFile::open(std::string_view file, std::string_view version, std::string_view useropts)
{
    const char *printerName;
    PPDValue fileVersion;

    // Check if printer information was previously opened
    if (_dest || _dinfo) {
        ERRORMSG(_("Internal warning: printer info was previously opened. Please "
            "close it before opening a new one."));
        close();
    }

    if (!file.empty()) _ppdPath = file;

    // Get the printer name from the environment
    printerName = getenv("PRINTER");
    if (!printerName)
        printerName = getenv("CUPS_DEST");

    _dest.reset(cupsGetNamedDest(CUPS_HTTP_DEFAULT, printerName, NULL));
    if (!_dest) {
        // Fallback: try to find any destination if PRINTER is not set
        _dest.reset(cupsGetNamedDest(CUPS_HTTP_DEFAULT, NULL, NULL));
    }

    if (!_dest) {
        WARNMSG(_("Cannot find printer destination (PRINTER=%s). Proceeding with defaults."), 
            printerName ? printerName : "NULL");
    } else {
        _dinfo.reset(cupsCopyDestInfo(CUPS_HTTP_DEFAULT, _dest.get()));
        if (!_dinfo) {
            WARNMSG(_("Cannot get destination info for printer %s. Proceeding with defaults."), _dest->name);
        }
    }

    // Direct PPD file loading fallback
    if (!file.empty()) {
        _ppd.reset(ppdOpenFile(file.data()));
        if (!_ppd) {
            ERRORMSG(_("Cannot open PPD file %s: %s"), file.data(), cupsLastErrorString());
            return SP::Unexpected(SP::Error::PPDOpenError);
        } else {
            ppdMarkDefaults(_ppd.get());
        }
    }

    // Parse user options
    _num_options = cupsParseOptions(useropts.data(), 0, &_options);

    // Check version
    fileVersion = get("FileVersion");
    if (!fileVersion.isNull() && fileVersion != version) {
        ERRORMSG(_("Invalid PPD version: %s (Expected: %s)"), (const char*)fileVersion, version.data());
        return SP::Unexpected(SP::Error::PPDVersionMismatch);
    }

    return {};
}

void PPDFile::close()
{
    _dinfo.reset();
    _dest.reset();
    _ppd.reset();
    if (_options) {
        cupsFreeOptions(_num_options, _options);
        _options = nullptr;
        _num_options = 0;
    }
}

/*
 * Obtention d'une donnée
 * Get a value
 */
PPDValue PPDFile::get(std::string_view name, std::string_view opt)
{
    const char *valStr = nullptr;
    PPDValue val;

    // 1. Search in job-specific options first
    valStr = cupsGetOption(name.data(), _num_options, _options);

    // 2. Search in destination info (defaults/attributes)
    if (!valStr && _dinfo) {
        if (!opt.empty()) {
            // It might be a choice for a specific option
            ipp_attribute_t *attr = cupsFindDestSupported(CUPS_HTTP_DEFAULT, 
                                        _dest.get(), _dinfo.get(), name.data());
            if (attr) {
                // Return default value if present
                valStr = cupsGetOption(name.data(), _dest->num_options, _dest->options);
            }
        } else {
            // Check for general IPP attributes
            valStr = cupsGetOption(name.data(), _dest->num_options, _dest->options);
            if (!valStr) {
                std::string cupsName = std::string("cups-") + std::string(name);
                valStr = cupsGetOption(cupsName.c_str(), _dest->num_options, 
                            _dest->options);
            }
        }
    }

    // 3. Fallback to direct PPD file loading
    if (!valStr && _ppd) {
        ppd_choice_t *choice = ppdFindMarkedChoice(_ppd.get(), name.data());
        if (choice) {
            valStr = choice->choice;
        } else {
            ppd_option_t *option = ppdFindOption(_ppd.get(), name.data());
            if (option) {
                valStr = option->defchoice;
            } else {
                // Finally, look for attributes (e.g. *QPDL BandSize)
                ppd_attr_t *attr = nullptr;
                if (!opt.empty()) {
                    attr = ppdFindAttr(_ppd.get(), opt.data(), name.data());
                } else {
                    attr = ppdFindAttr(_ppd.get(), name.data(), nullptr);
                }

                if (attr && attr->value) {
                    valStr = attr->value;
                    // Attributes in PPD often have quotes, e.g., "128"
                    if (valStr[0] == '"') {
                        std::string_view sv(valStr);
                        if (sv.size() >= 2 && sv.back() == '"') {
                            val.set(sv.substr(1, sv.size() - 2));
                            val.setPreformatted(); 
                            return val;
                        }
                    }
                } else if (!opt.empty() && !name.empty()) {
                    // Fallback: try reversed just in case libcups/PPD is weird
                    attr = ppdFindAttr(_ppd.get(), name.data(), opt.data());
                    if (attr && attr->value) {
                        valStr = attr->value;
                    }
                }

            }
        }
    }

    if (valStr)
        val.set(valStr);

    return val;
}

PPDValue PPDFile::getPageSize(std::string_view name)
{
    PPDValue val;
    cups_size_t size;

    if (_dinfo && cupsGetDestMediaByName(CUPS_HTTP_DEFAULT, _dest.get(), _dinfo.get(), name.data(), 
            CUPS_MEDIA_FLAGS_DEFAULT, &size)) {
        // CUPS size is in 100ths of a millimeter. Convert to points (1/72 inch).
        float w = (static_cast<float>(size.width) / 2540.0f) * 72.0f;
        float h = (static_cast<float>(size.length) / 2540.0f) * 72.0f;
        float lm = (static_cast<float>(size.left) / 2540.0f) * 72.0f;
        float bm = (static_cast<float>(size.bottom) / 2540.0f) * 72.0f;
        val.set(w, h, lm, bm);
    } else if (_ppd) {
        // Fallback to direct PPD page size resolution
        ppd_size_t *psize = ppdPageSize(_ppd.get(), name.data());
        if (psize) {
            val.set(psize->width, psize->length, psize->left, psize->bottom);
        }
    }

    return val;
}



/*
 * Gestion des valeurs du PPD
 * PPD values management
 */
PPDFile::Value::Value() : 
    _width(0.0f), _height(0.0f), _marginX(0.0f), _marginY(0.0f)
{
}

PPDFile::Value::Value(std::string_view value) : 
    _raw(value), _out(_raw), _hasValue(true), 
    _width(0.0f), _height(0.0f), _marginX(0.0f), _marginY(0.0f)
{
}

PPDFile::Value::~Value() = default;

PPDFile::Value& PPDFile::Value::set(std::string_view value)
{
    _preformatted.clear();
    _raw = value;
    _out = _raw;
    _hasValue = true;

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
    if (_raw.empty())
        return *this;

    const char *str = _raw.c_str();

    _preformatted.clear();
    _preformatted.reserve(_raw.size());

    while (*str) {
        if (*str == '<' && strlen(str) >= 3 && isxdigit(*(str+1))) {
            char temp[3] = {0, 0, 0};

            str++;
            temp[0] = *str;
            str++;
            if (*str != '>' && (!isxdigit(*str) || *(str + 1) != '>')) {
                _preformatted += '<';
                _preformatted += temp[0];
                continue;
            }
            if (*str != '>') {
                temp[1] = *str;
                str++;
            }
            _preformatted += static_cast<char>(strtol(temp, nullptr, 16));
            if (*str == '>')
                str++;
            continue;
        }
        _preformatted += *str;
        str++;
    }
    _out = _preformatted;

    return *this;
}

std::string PPDFile::Value::deepCopy() const
{
    if (_out.empty())
        return "";
    return std::string(_out);
}

bool PPDFile::Value::isTrue() const
{
    if (_out.empty())
        return false;

    auto compare = [this](std::string_view target) {
        return std::ranges::equal(_out, target, [](char c1, char c2) {
            return std::tolower(static_cast<unsigned char>(c1)) == 
                   std::tolower(static_cast<unsigned char>(c2));
        });
    };

    if (_out == "1" || compare("true") || 
        compare("enable") || compare("enabled") || 
        compare("yes") || compare("on"))
        return true;
    return false;
}

PPDFile::Value::operator unsigned long() const {
    unsigned long val = 0;
    if (!_out.empty()) {
        std::from_chars(_out.data(), _out.data() + _out.size(), val);
    }
    return val;
}

PPDFile::Value::operator long() const {
    long val = 0;
    if (!_out.empty()) {
        std::from_chars(_out.data(), _out.data() + _out.size(), val);
    }
    return val;
}

PPDFile::Value::operator float() const {
    float val = 0.0f;
    if (!_out.empty()) {
        try {
            val = std::stof(_out);
        } catch (...) {}
    }
    return val;
}

PPDFile::Value::operator double() const {
    double val = 0.0;
    if (!_out.empty()) {
        try {
            val = std::stod(_out);
        } catch (...) {}
    }
    return val;
}

PPDFile::Value::operator long double() const {
    long double val = 0.0;
    if (!_out.empty()) {
        try {
            val = std::stold(_out);
        } catch (...) {}
    }
    return val;
}

bool PPDFile::Value::operator == (std::string_view val) const
{
    if (_out.empty())
        return false;
    return std::ranges::equal(_out, val, [](char c1, char c2) {
        return std::tolower(static_cast<unsigned char>(c1)) == 
               std::tolower(static_cast<unsigned char>(c2));
    });
}

bool PPDFile::Value::operator == (const char *val) const
{
    if (_out.empty() || !val)
        return false;
    return (*this) == std::string_view(val);
}

bool PPDFile::Value::operator != (std::string_view val) const
{
    return !((*this) == val);
}

bool PPDFile::Value::operator != (const char *val) const
{
    return !((*this) == val);
}



/*
 * Copy constructor
 */
PPDFile::Value::Value(const PPDFile::Value &val)
{
    _raw = val._raw;
    _preformatted = val._preformatted;
    _out = val._preformatted.empty() ? _raw : _preformatted;
    _hasValue = val._hasValue;
    _width = val._width;
    _height = val._height;
    _marginX = val._marginX;
    _marginY = val._marginY;
}

PPDFile::Value& PPDFile::Value::operator = (const PPDFile::Value &val)
{
    if (this == &val)
        return *this;
    _raw = val._raw;
    _preformatted = val._preformatted;
    _out = val._preformatted.empty() ? _raw : _preformatted;
    _hasValue = val._hasValue;
    _width = val._width;
    _height = val._height;
    _marginX = val._marginX;
    _marginY = val._marginY;
    return *this;
}


/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

