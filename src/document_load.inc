/*
 * 	    document.cpp              (C) 2006-2008, Aurélien Croc (AP²C)
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
#include <cstring>
#include <array>
#include <algorithm>
#include <cmath>
#include "sp_portable.h"
#include "page.h"
#include "document.h"
#include "band.h"
#include "errlog.h"
#include "request.h"


/*
 * Ouverture du fichier contenant la requête
 * Open the file which contains the job
 */
SP::Result<> Document::load([[maybe_unused]] const Request& request)
{
    _currentPage = 1;
    _lastPage = false;
    _raster.reset(cupsRasterOpen(SP_STDIN_FILENO, CUPS_RASTER_READ));
    if (!_raster) {
        ERRORMSG(_("Cannot open job"));
        return SP::Unexpected(SP::Error::RasterOpenError);
    }
    return {};
}

/*
 * Extraction d'une nouvelle page de la requête
 * Exact a new job page
 */
