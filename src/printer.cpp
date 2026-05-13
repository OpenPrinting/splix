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
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  $Id$
 * 
 */
#include "printer.h"
#include <ctime>
#include <cstring>
#include <cstdio>
#include "errlog.h"
#include "request.h"
#include "ppdfile.h"
#include "sp_portable.h"

namespace {
    /** Case-insensitive string_view comparison (ASCII only). */
    bool compare(std::string_view a, std::string_view b) {
        if (a.length() != b.length()) return false;
        for (size_t i = 0; i < a.length(); ++i)
            if (std::tolower(static_cast<unsigned char>(a[i])) !=
                std::tolower(static_cast<unsigned char>(b[i]))) return false;
        return true;
    }
} // anonymous namespace



/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
Printer::Printer() = default;

Printer::~Printer() = default;



/*
 * Chargement des informations sur l'imprimante
 * Load the printer information
 */
bool Printer::loadInformation(const Request& request)
{
    const char *paperType, *paperSource;
    PPDValue value;

    // Get some printer information
    if (request.ppd()->get("BandSize", "QPDL").isNull()) {
        ERRORMSG(_("Unknown band size. Operation aborted."));
        return false;
    }
    _bandHeight = request.ppd()->get("BandSize", "QPDL");
    _specialBandWidth = request.ppd()->get("SpecialBandWidth", "QPDL");
    _fixedBandWidth = request.ppd()->get("FixedBandWidth", "QPDL");
    _packetSize = request.ppd()->get("PacketSize", "QPDL");
    _packetSize *= 1024;
    _manufacturer = std::string(request.ppd()->get("Manufacturer"));
    _model = std::string(request.ppd()->get("ModelName"));
    _color = request.ppd()->get("ColorDevice");
    _qpdlVersion = request.ppd()->get("QPDLVersion", "QPDL");
    if (!_qpdlVersion || (_qpdlVersion > 3 && _qpdlVersion != 5)) {
        ERRORMSG(_("Invalid QPDL version. Operation aborted."));
        return false;
    }
    value = request.ppd()->get("DocHeaderValues", "General");
    value.setPreformatted();
    if (value.isNull() || value.deepCopy().length() < 3) {
        ERRORMSG(_("Unknown header values. Operation aborted."));
        return false;
    }
    std::string headerVals = value.deepCopy();
    _unknownByte1 = headerVals[0];
    _unknownByte2 = headerVals[1];
    _unknownByte3 = headerVals[2];

    // Get PJL information
    value = request.ppd()->get("BeginPJL", "PJL");
    value.setPreformatted();
    if (value.isNull()) {
        ERRORMSG(_("No PJL header found. Operation aborted."));
        return false;
    }
    _beginPJL = value.deepCopy();
    value = request.ppd()->get("EndPJL", "PJL");
    value.setPreformatted();
    if (value.isNull()) {
        ERRORMSG(_("No PJL footer found. Operation aborted."));
        return false;
    }
    _endPJL = value.deepCopy();

    // Get the paper information
    auto mediaSizeValue = request.ppd()->get("MediaSize");
    const char* rawPaperType = mediaSizeValue;
    if (!rawPaperType) {
        mediaSizeValue = request.ppd()->get("PageSize");
        rawPaperType = mediaSizeValue;
    }
    if (!rawPaperType) {
        ERRORMSG(_("Cannot get paper size information. Operation aborted."));
        return false;
    }

    std::string_view paperTypeView(rawPaperType);

    if (compare(paperTypeView, "Letter")) _paperType = 0;
    else if (compare(paperTypeView, "Legal")) _paperType = 1;
    else if (compare(paperTypeView, "A4")) _paperType = 2;
    else if (compare(paperTypeView, "Executive")) _paperType = 3;
    else if (compare(paperTypeView, "Ledger")) _paperType = 4;
    else if (compare(paperTypeView, "A3")) _paperType = 5;
    else if (compare(paperTypeView, "Env10")) _paperType = 6;
    else if (compare(paperTypeView, "Monarch")) _paperType = 7;
    else if (compare(paperTypeView, "C5")) _paperType = 8;
    else if (compare(paperTypeView, "DL")) _paperType = 9;
    else if (compare(paperTypeView, "B4")) _paperType = 10;
    else if (compare(paperTypeView, "B5")) _paperType = 11;
    else if (compare(paperTypeView, "EnvISOB5")) _paperType = 12;
    else if (compare(paperTypeView, "Postcard")) _paperType = 14;
    else if (compare(paperTypeView, "DoublePostcardRotated")) _paperType = 15;
    else if (compare(paperTypeView, "A5")) _paperType = 16;
    else if (compare(paperTypeView, "A6")) _paperType = 17;
    else if (compare(paperTypeView, "B6")) _paperType = 18;
    else if (compare(paperTypeView, "Custom")) _paperType = 21;
    else if (compare(paperTypeView, "C6")) _paperType = 23;
    else if (compare(paperTypeView, "Folio")) _paperType = 24;
    else if (compare(paperTypeView, "EnvPersonal")) _paperType = 25;
    else if (compare(paperTypeView, "Env9")) _paperType = 26;
    else if (compare(paperTypeView, "3x5")) _paperType = 27;
    else if (compare(paperTypeView, "Oficio")) _paperType = 28;
    else if (compare(paperTypeView, "Statement")) _paperType = 30;
    else if (compare(paperTypeView, "roc8k")) _paperType = 34;
    else if (compare(paperTypeView, "roc16k")) _paperType = 35;
    else {
        ERRORMSG(_("Invalid paper size \"%s\". Operation aborted."), rawPaperType);
        return false;
    }

    value = request.ppd()->getPageSize(rawPaperType);
    if (value.width() == 0. || value.height() == 0.) {
        ERRORMSG(_("Null paper size \"%s\" found. Operation aborted."), rawPaperType);
        return false;
    }
    _pageWidth = value.width();
    _pageHeight = value.height();
    _hardMarginX = value.marginX();
    _hardMarginY = value.marginY();

    auto inputSlotValue = request.ppd()->get("InputSlot");
    const char* rawPaperSource = inputSlotValue;
    if (!rawPaperSource) {
        inputSlotValue = request.ppd()->get("DefaultInputSlot");
        rawPaperSource = inputSlotValue;
        if (!rawPaperSource) {
            rawPaperSource = "Auto";
            ERRORMSG(_("Cannot get input slot information."));
        }
    }

    std::string_view sourceView(rawPaperSource);
    if (compare(sourceView, "Auto")) _paperSource = 1;
    else if (compare(sourceView, "Manual")) _paperSource = 2;
    else if (compare(sourceView, "Multi")) _paperSource = 3;
    else if (compare(sourceView, "Upper")) _paperSource = 4;
    else if (compare(sourceView, "Lower")) _paperSource = 5;
    else if (compare(sourceView, "Envelope")) _paperSource = 6;
    else if (compare(sourceView, "Tray3")) _paperSource = 7;
    else {
        ERRORMSG(_("Invalid paper source \"%s\". Operation aborted."), rawPaperSource);
        return false;
    }

    DEBUGMSG(_("%s printer %s with QPDL v. %lu"), _color ? "Color" : 
        "Monochrome", _model.c_str(), _qpdlVersion);

    return true;
}

bool Printer::sendPJLHeader(const Request& request,
                         unsigned long    compression,
                         unsigned long    xResolution,
                         unsigned long    yResolution ) const
{
    const char *reverse;
    struct tm timeinfo_buf;
    struct tm *timeinfo;
    time_t timestamp;

    time(&timestamp);
    timeinfo = SP::portable_localtime(&timestamp, &timeinfo_buf);

    printf("%s", _beginPJL.c_str());

    if (0x15 == compression) {
        printf("@PJL COMMENT USERNAME=\"Username: %s\"\n", request.userName());
        printf("@PJL COMMENT DOCNAME=\"%s\"\n", request.jobTitle());
        printf("@PJL JOB NAME=\"%s\"\n", request.jobTitle());
    }

    // Information about the job
    printf("@PJL DEFAULT SERVICEDATE=%04u%02u%02u\n", 1900+timeinfo->tm_year,
        timeinfo->tm_mon+1, timeinfo->tm_mday);
    printf("@PJL SET USERNAME=\"%s\"\n", request.userName());
    printf("@PJL SET JOBNAME=\"%s\"\n", request.jobTitle());

    if (0x15 == compression)
        printf("@PJL SET MULTIBINMODE=%s\n", "PRINTERDEFAULT");

   // Set some printer options
    if (!request.ppd()->get("EconoMode").isNull() && 
        request.ppd()->get("EconoMode") != "0")
        printf("@PJL SET ECONOMODE=%s\n", static_cast<const char*>(request.ppd()->
                get("EconoMode")));
    if (request.ppd()->get("PowerSave")) {
        printf("@PJL SET POWERSAVE=%s\n", 
                static_cast<const char*>(request.ppd()->get("PowerSave")));
    }

    if (request.ppd()->get("JamRecovery").isTrue())
        printf("@PJL SET JAMRECOVERY=ON\n");
    else
        printf("@PJL SET JAMRECOVERY=OFF\n");
    if (request.printer()->color()) {
        auto colorModelValue = request.ppd()->get("ColorModel");
        const char* colorModel = colorModelValue;
        if (colorModel && compare(colorModel, "CMYK"))
            printf("@PJL SET COLORMODE=COLOR\n");
        else
            printf("@PJL SET COLORMODE=MONO\n");
    }

    if (0x15 == compression) {
        printf("@PJL SET RESOLUTION=%lu\n", yResolution);
        if ((600 == xResolution) && (600 == yResolution))
            printf("@PJL SET IMAGEQUALITY=0\n");
        if ((1200 == xResolution) && (600 == yResolution))
            printf("@PJL SET IMAGEQUALITY=1\n");
        printf("@PJL SET RGBCOLOR=%s\n", "STANDARD");
    }

    // Information about the duplex
    reverse = request.reverseDuplex() ? "REVERSE_" : "";
    switch (request.duplex()) {
        case Request::Simplex:
            printf("@PJL SET DUPLEX=OFF\n");
            break;
        case Request::LongEdge:
            printf("@PJL SET DUPLEX=ON\n");
            printf("@PJL SET BINDING=%sLONGEDGE\n", reverse);
            break;
        case Request::ShortEdge:
            printf("@PJL SET DUPLEX=ON\n");
            printf("@PJL SET BINDING=%sSHORTEDGE\n", reverse);
            break;
        case Request::ManualLongEdge:
            printf("@PJL SET DUPLEX=MANUAL\n");
            printf("@PJL SET BINDING=LONGEDGE\n");
            break;
        case Request::ManualShortEdge:
            printf("@PJL SET DUPLEX=MANUAL\n");
            printf("@PJL SET BINDING=SHORTEDGE\n");
            break;
    }
 
    // Set some job options
    if (request.ppd()->get("MediaType").isNull())
        printf("@PJL SET PAPERTYPE=OFF\n");
    else
        printf("@PJL SET PAPERTYPE=%s\n", static_cast<const char*>(request.ppd()->
                get("MediaType")));
    if (request.ppd()->get("Altitude")) {
        printf("@PJL SET ALTITUDE=%s\n", static_cast<const char*>(request.ppd()->
                get("Altitude")));
    }
    if (request.ppd()->get("TonerDensity")) {
        printf("@PJL SET DENSITY=%s\n", static_cast<const char*>(request.ppd()->
                get("TonerDensity")));
    }
    if (request.ppd()->get("SRTMode")) {
        printf("@PJL SET RET=%s\n", static_cast<const char*>(request.ppd()->
                get("SRTMode")));
    }

    if (0x15 == compression) {
        printf("@PJL SET BANNERSHEET=%s\n", "OFF");
        printf("@PJL SET TIMESTAMP=%s\n", "OFF");
    }

    printf("@PJL ENTER LANGUAGE = QPDL\n");
    fflush(stdout);

    return true;
}

bool Printer::sendPJLFooter([[maybe_unused]] const Request& request) const
{
    printf("%s", _endPJL.c_str());
    fflush(stdout);

    return true;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

