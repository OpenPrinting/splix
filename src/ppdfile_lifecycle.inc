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
        ERRORMSG(_("Invalid PPD version: %s (Expected: %s)"),
            static_cast<const char*>(fileVersion), version.data());
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
