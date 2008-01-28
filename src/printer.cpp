/*
 * 	    printer.cpp               (C) 2006-2008, Aurélien Croc (AP²C)
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
#include "printer.h"
#include <string.h>
#include "request.h"
#include "ppdfile.h"
#include "errlog.h"



/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
Printer::Printer()
{
    _manufacturer = NULL;
    _model = NULL;
    _beginPJL = NULL;
    _endPJL = NULL;
}

Printer::~Printer()
{
    if (_manufacturer)
        delete[] _manufacturer;
    if (_model)
        delete [] _model;
    if (_beginPJL)
        delete [] _beginPJL;
    if (_endPJL)
        delete [] _endPJL;
}



/*
 * Chargement des informations sur l'imprimante
 * Load the printer information
 */
bool Printer::loadInformation(const Request& request)
{
    const char *paperType, *paperSource;
    PPDValue value;

    // Get some printer information
    /** @todo bandHeight */
    _bandHeight = 0x80; // XXX XXX XXX XXX XXX TODO TODO TODO TODO
    _manufacturer = request.ppd()->get("Manufacturer").deepCopy();
    _model = request.ppd()->get("ModelName").deepCopy();
    _color = request.ppd()->get("ColorDevice");
    _qpdlVersion = request.ppd()->get("QPDLVersion", "QPDL");
    if (!_qpdlVersion || _qpdlVersion > 2) {
        ERRORMSG(_("Invalid QPDL version. Operation aborted."));
        return false;
    }
    value = request.ppd()->get("docHeaderValues", "General");
    value.setPreformatted();
    if (value.isNull()) {
        ERRORMSG(_("Unknown header values. Operation aborted."));
        return false;
    }
    _unknownByte1 = ((const char *)value)[0];
    _unknownByte2 = ((const char *)value)[1];
    _unknownByte3 = ((const char *)value)[2];

    // Get PJL information
    value = request.ppd()->get("beginPJL", "PJL");
    value.setPreformatted();
    if (value.isNull()) {
        ERRORMSG(_("No PJL header found. Operation aborted."));
        return false;
    }
    _beginPJL = value.deepCopy();
    value = request.ppd()->get("endPJL", "PJL");
    value.setPreformatted();
    if (value.isNull()) {
        ERRORMSG(_("No PJL footer found. Operation aborted."));
        return false;
    }
    _endPJL = value.deepCopy();

    // Get the paper information
    paperType = request.ppd()->get("MediaSize");
    if (!paperType)
        paperType = request.ppd()->get("PageSize");
    if (!paperType) {
        ERRORMSG(_("Cannot get paper size information. Operation aborted."));
        return false;
    }
    if (!(strcasecmp(paperType, "Letter"))) _paperType = 0;
    else if (!(strcasecmp(paperType, "Legal"))) _paperType = 1;
    else if (!(strcasecmp(paperType, "A4"))) _paperType = 2;
    else if (!(strcasecmp(paperType, "Executive"))) _paperType = 3;
    else if (!(strcasecmp(paperType, "Ledger"))) _paperType = 4;
    else if (!(strcasecmp(paperType, "A3"))) _paperType = 5;
    else if (!(strcasecmp(paperType, "Env10"))) _paperType = 6;
    else if (!(strcasecmp(paperType, "Monarch"))) _paperType = 7;
    else if (!(strcasecmp(paperType, "C5"))) _paperType = 8;
    else if (!(strcasecmp(paperType, "DL"))) _paperType = 9;
    else if (!(strcasecmp(paperType, "B4"))) _paperType = 10;
    else if (!(strcasecmp(paperType, "B5"))) _paperType = 11;
    else if (!(strcasecmp(paperType, "EnvISOB5"))) _paperType = 12;
    else if (!(strcasecmp(paperType, "A5"))) _paperType = 16;
    else if (!(strcasecmp(paperType, "A6"))) _paperType = 17;
    else if (!(strcasecmp(paperType, "EnvISOB6"))) _paperType = 18;
    else if (!(strcasecmp(paperType, "C6"))) _paperType = 23;
    else if (!(strcasecmp(paperType, "Folio"))) _paperType = 24;
    else {
        ERRORMSG(_("Invalid paper size \"%s\". Operation aborted."), paperType);
        return false;
    }

    paperSource = request.ppd()->get("InputSlot");
    if (!paperSource) {
        ERRORMSG(_("Cannot get input slot information. Operation aborted."));
        return false;
    }
    if (!(strcasecmp(paperSource, "Auto"))) _paperSource = 1;
    else if (!(strcasecmp(paperSource, "Manual"))) _paperSource = 2;
    else if (!(strcasecmp(paperSource, "Multi"))) _paperSource = 3;
    else if (!(strcasecmp(paperSource, "Upper"))) _paperSource = 4;
    else if (!(strcasecmp(paperSource, "Lower"))) _paperSource = 5;
    else if (!(strcasecmp(paperSource, "Envelope"))) _paperSource = 6;
    else if (!(strcasecmp(paperSource, "Tray3"))) _paperSource = 7;
    else {
        ERRORMSG(_("Invalid paper source \"%s\". Operation aborted."), 
            paperSource);
        return false;
    }

    DEBUGMSG(_("%s printer %s with QPDL v. %lu"), _color ? "Color" : 
        "Monochrome", _model, _qpdlVersion);

    return true;
}

bool Printer::sendPJLHeader(const Request& request) const
{
    printf("%s", _beginPJL);

    /** @todo add the PJL header */

    printf("@PJL ENTER LANGUAGE = QPDL\n");
    fflush(stdout);

    return true;
}

bool Printer::sendPJLFooter(const Request& request) const
{
    printf("%s", _endPJL);

    return true;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

