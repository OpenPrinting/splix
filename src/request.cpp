/*
 * 	    request.cpp               (C) 2006-2008, Aurélien Croc (AP²C)
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
#include "request.h"
#include "errlog.h"
#include "ppdfile.h"

/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
Request::Request()
{
}

Request::~Request()
{
}



/*
 * Chargement d'une requête
 * Load a request
 */
bool Request::loadRequest(PPDFile* ppd, const char *jobname, 
    const char *username, const char *jobtitle, unsigned long copiesNr)
{
    if (!ppd) {
        ERRORMSG(_("Request: NULL PPD handle given"));
        return false;
    }

    _ppd = ppd;
    _jobname = jobname ? jobname : _("Unknown");
    _username = username ? username : getenv("USER");
    _jobtitle = jobtitle ? jobtitle : _("Unknown job title");
    _copiesNr = copiesNr;

    /** @todo Get the real duplex mode */
    //_duplex = Simplex;
    _duplex = ManualLongEdge;
    _reverseDuplex = false;

    if (!_printer.loadInformation(*this)) {
        ERRORMSG(_("Request: cannot load printer information"));
        return false;
    }
    
    return true;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

