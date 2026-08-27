static SP::Result<> _compressWholePage(const Request& request, Page* page)
{
    uint32_t hardMarginX, hardMarginXInB, hardMarginY, lineWidthInB;
    uint32_t pageWidth, bandHeight, planeHeight, pageHeight, index;
    uint32_t bandNumber=0;
    std::vector<uint8_t> buffer;
    Band *raw_current = nullptr;
    Algo0x13 algo[4];

    if (!request.printer()) {
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    hardMarginX = (static_cast<uint32_t>(std::ceil(page->convertToXResolution(request.
        printer()->hardMarginX()))) + 7) & ~7;
    hardMarginY = static_cast<uint32_t>(std::ceil(page->convertToYResolution(request.printer()->
        hardMarginY())));
    hardMarginXInB = hardMarginX / 8;

    if (page->width() <= hardMarginX * 2 || page->height() <= hardMarginY * 2) {
        ERRORMSG(_("Page dimensions too small for margins"));
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    pageWidth = page->width() - hardMarginX * 2;
    pageHeight = page->height() - hardMarginY * 2;
    page->setWidth(pageWidth);
    page->setHeight(pageHeight);
    lineWidthInB = (pageWidth + 7) / 8;
    bandHeight = static_cast<uint32_t>(request.printer()->bandHeight());
    if (!bandHeight) {
        ERRORMSG(_("Invalid zero band height"));
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    // Alignment of the page height on band height
    planeHeight = ((pageHeight + bandHeight - 1) / bandHeight) * bandHeight;

    try {
        buffer.resize(lineWidthInB * planeHeight);
    } catch (const std::bad_alloc&) {
        return SP::Unexpected(SP::Error::MemoryError);
    }

    do {
        std::unique_ptr<Band> current = nullptr;
        for (uint32_t i=0; i < page->colorsNr(); i++) {
            const uint8_t *curPlane = page->planeBuffer(static_cast<uint8_t>(i));
            if (!curPlane) {
                 ERRORMSG(_("Plane %u has no data"), i);
                 return SP::Unexpected(SP::Error::LogicError);
            }
            SP::Result<std::unique_ptr<BandPlane>> planeRes = SP::Unexpected(SP::Error::None);

            index = hardMarginY * (lineWidthInB + 2 * hardMarginXInB) +
                hardMarginXInB;
            for (uint32_t y = 0; y < pageHeight; y++) {
                const uint32_t dstRowStart = y * lineWidthInB;
                const uint32_t srcRowStart = index + y * (lineWidthInB + 2 * hardMarginXInB);

                std::ranges::copy(std::span(curPlane + srcRowStart, lineWidthInB),
                                  buffer.begin() + dstRowStart);
            }
            std::fill(buffer.begin() + pageHeight * lineWidthInB, buffer.end(), 0);

            // Call the compression method
            planeRes = algo[i].compress(request, buffer, page->width(),
                planeHeight);
            if (planeRes) {
                std::unique_ptr<BandPlane> plane = std::move(*planeRes);
                plane->setColorNr(static_cast<uint8_t>(i + 1));
                if (!current)
                    current = std::make_unique<Band>(bandNumber, page->width(),
                        request.printer()->bandHeight());
                current->registerPlane(std::move(plane));
            } else if (planeRes.error() != SP::Error::None) {
                 return SP::Unexpected(planeRes.error());
            }
        }
        raw_current = current.get();
        if (current)
            page->registerBand(std::move(current));
        bandNumber++;
    } while (raw_current);

    page->flushPlanes();

    return {};
}
