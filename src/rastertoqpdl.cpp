/*
 * 	rastertoqpdl.cpp	(C) 2006-2007, Aurélien Croc (AP²C)
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
#include <cups/ppd.h>
#include <cups/cups.h>
#include "errlog.h"
#include "version.h"
#include "request.h"

ppd_file_t* openPPDFile(const char *ppdFile, const char *useropts)
{
    cups_option_t *options;
    ppd_attr_t *attr;
    ppd_file_t* ppd;
    int nr;

    // Open the PPD file
    ppd = ppdOpenFile(ppdFile);
    if (!ppd) {
        ERRORMSG(_("Cannot open PPD file %s"), ppdFile);
        return NULL;
    }

    // Mark the default values and the user options
    ppdMarkDefaults(ppd);
    nr = cupsParseOptions(useropts, 0, &options);
    cupsMarkOptions(ppd, nr, options);
    cupsFreeOptions(nr, options);

    // Check if the PPD version is compatible with this filter
    attr = ppdFindAttr(ppd, "FileVersion", NULL);
    if (!attr) {
        ERRORMSG(_("No FileVersion found in the PPD file: invalid "
            "PPD file"));
        ppdClose(ppd);
        return NULL;
    }
    if (strcmp(attr->value, VERSION)) {
        ERRORMSG(_("Invalid PPD file version: Splix V. %s but the PPD file "
            "is designed for SpliX V. %s"), VERSION, attr->value);
        ppdClose(ppd);
        return NULL;
    }

    return ppd;
}

int main(int argc, char **argv)
{
    const char *ppdFile = "../../splix/ppd/ml2250fr.ppd";
    ppd_file_t* ppd;
    Request request;

    // Open the PPD file
    if (!(ppd = openPPDFile(ppdFile, "")))
        return 1;

    // Load the request
    if (!request.loadRequest(ppd, "ID-0001", "aurelien", "Job de test", 1)) {
        ppdClose(ppd);
        return 2;
    }

    DEBUGMSG("Fabricant %s, model %s", request.printer()->manufacturer(), 
        request.printer()->model());

    ppdClose(ppd);
    return 0;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

