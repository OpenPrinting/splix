SP::Result<> renderPage(const Request& request, Page* page, bool lastPage)
{
    unsigned char duplex=0, tumble=0, paperSource=1;
    unsigned long width, height;
    std::array<unsigned char, 0x11> header;
    const Band* band;
    SP::Result<> (*selectedRenderBand)(const Request&, const Band*, bool);


    if (!page) {
        ERRORMSG(_("Try to render a NULL page"));
        return SP::Unexpected(SP::Error::InconsistentData);
    }

    // Get the duplex values
    paperSource = request.printer()->paperSource();
    switch (request.duplex()) {
        case Request::Simplex:
            /* Observed a value of 0 for 0x15 printers. */
            duplex = (0x15 == page->compression())? 0 : 1;
            tumble = 0;
            break;
        case Request::LongEdge:
            duplex = 1;
            tumble = page->pageNr() % 2;
            break;
        case Request::ShortEdge:
            duplex = 0;
            tumble = page->pageNr() % 2;
            break;
        case Request::ManualLongEdge:
            duplex = 0;
            tumble = page->pageNr() % 2;
            if (tumble && !lastPage)
                paperSource = 3; // Multi source
            break;
        case Request::ManualShortEdge:
            duplex = 0;
            tumble = page->pageNr() % 2;
            if (tumble && !lastPage)
                paperSource = 3; // Multi source
            /** @todo what about the Short edge? The page isn't rotated?  */
            break;
    }

    width = page->width();
    height = page->height();

    if (0x15 == page->compression()) {
        selectedRenderBand = &_renderJBIGBand;
    } else {
        selectedRenderBand = &_renderBand;
    }

    if (request.printer()->fixedBandWidth() || 0x15 == page->compression()) {
        // For CLP-310/315 and fixed bandwidth printers, multiply page
        // dimensions in inches by 300.
        width = static_cast<unsigned long>(ceil(300 * (request.printer()->pageWidth() / 72.0)));
        height = static_cast<unsigned long>(ceil(300 * (request.printer()->pageHeight() / 72.0)));
    }

    // Send the page header
    header[0x0] = 0;                                // Signature
    header[0x1] = static_cast<unsigned char>(page->yResolution() / 100);        // Y Resolution
    header[0x2] = static_cast<unsigned char>(page->copiesNr() >> 8);            // Number of copies 8-15
    header[0x3] = static_cast<unsigned char>(page->copiesNr());                 // Number of copies 0-7
    header[0x4] = static_cast<unsigned char>(request.printer()->paperType());   // Paper type
    header[0x5] = static_cast<unsigned char>(width >> 8);                       // Printable area width
    header[0x6] = static_cast<unsigned char>(width);                            // Printable area width
    header[0x7] = static_cast<unsigned char>(height >> 8);                      // Printable area height
    header[0x8] = static_cast<unsigned char>(height);                           // Printable area height
    header[0x9] = static_cast<unsigned char>(paperSource);                      // Paper source
    header[0xa] = static_cast<unsigned char>(request.printer()->unknownByte1());// Always 0 in ULD
    header[0xb] = duplex;                           // Duplex
    header[0xc] = tumble;                           // Tumble
    header[0xd] = static_cast<unsigned char>(request.printer()->unknownByte2());// Always 0 in ULD
    header[0xe] = static_cast<unsigned char>(request.printer()->qpdlVersion()); // QPDL Version
    header[0xf] = static_cast<unsigned char>(request.printer()->unknownByte3());// Colorplanes
    header[0x10] = static_cast<unsigned char>(page->xResolution() / 100);       // X Resolution
    if (auto res = _safeWrite(std::span(header.data(), 0x11)); !res) return res;

    // Send auxiliary records for clp-315 printers.
    if (0x15 == page->compression()) {
        auto auxRes = _outputAuxRecords(page);
        if (!auxRes)
            return auxRes;
    }

    // Send the page bands
    band = page->firstBand();
    while (band) {
        auto res = selectedRenderBand(request, band, page->colorsNr() == 1);
        if (!res)
            return res;
        band = band->sibling();
    }

    // Send the page footer
    header[0x0] = 1;                                // Signature
    header[0x1] = static_cast<unsigned char>(page->copiesNr() >> 8);            // Number of copies 8-15
    header[0x2] = static_cast<unsigned char>(page->copiesNr());                 // Number of copies 0-7
    if (auto res = _safeWrite(std::span(header.data(), 0x3)); !res) return res;

    return {};
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */
