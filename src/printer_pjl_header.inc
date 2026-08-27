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
