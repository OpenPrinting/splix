static SP::Result<> _compressBandedJBIGPage(const Request& request, Page* page)
{
    uint32_t index=0, pageHeight, lineWidthInB, bandHeight = 128;
    uint32_t bufferWidth, specialBufferWidthInB, bandSize, hardMarginXInB=13, hardMarginY=100;
    const uint8_t *planes[4];
    std::vector<uint8_t> band[4];
    uint32_t bandNumber=0, xLimitInB, bufferWidthInB;
    auto algo = std::make_unique<Algo0x15>();

    if (!request.printer()) {
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    /* Image trimming are done from hardware margins defined in the ppd. */
    hardMarginXInB = (static_cast<uint32_t>(std::ceil(page->convertToXResolution(request.
        printer()->hardMarginX()))) + 7) / 8;
    hardMarginY = static_cast<uint32_t>(std::ceil(page->convertToYResolution(request.printer()->
        hardMarginY())));

    if (page->height() <= hardMarginY) {
         ERRORMSG(_("Page height (%u) is smaller than hard margin (%u)"), page->height(), hardMarginY);
         return SP::Unexpected(SP::Error::InvalidArgument);
    }
    pageHeight = page->height() - hardMarginY;

    lineWidthInB = ((page->width()) + 7) / 8;
    // Compute the buffer width that is nearest multiple of 256 to the original.
    bufferWidth = page->width() & ~255;
    if ((bufferWidth + 128) < page->width())
      bufferWidth += 256;
    bandHeight = static_cast<uint32_t>(request.printer()->bandHeight());

    if (request.printer()->specialBandWidth()) {
        specialBufferWidthInB = static_cast<uint32_t>(_get2020BandWidthInB(request.printer()->paperType(),
                                                page->xResolution(),
                                                page->width()));
        if (specialBufferWidthInB) bufferWidth = specialBufferWidthInB * 8;
    }
    // Update the page height.
    page->setHeight(pageHeight);
    // Update the page width.
    page->setWidth(bufferWidth);
    bufferWidthInB = (bufferWidth + 7) / 8;

    // Safety check for bandSize
    if (static_cast<uint64_t>(bufferWidthInB) * bandHeight > 0x20000000) {
        ERRORMSG(_("Calculated jbig band size is too large"));
        return SP::Unexpected(SP::Error::MemoryError);
    }
    bandSize = bufferWidthInB * bandHeight;
    index = hardMarginY * lineWidthInB;

    try {
        for (uint32_t i=0; i < page->colorsNr(); i++) {
            band[i].resize(bandSize);
            planes[i] = page->planeBuffer(static_cast<uint8_t>(i));
            if (!planes[i]) {
                ERRORMSG(_("JBIG Plane %u has no data"), i);
                return SP::Unexpected(SP::Error::LogicError);
            }
        }
    } catch (const std::bad_alloc&) {
        return SP::Unexpected(SP::Error::MemoryError);
    }

    /*
       Here, limit the width of the copied image to the buffer, as the buffer
       width varies and can lead to 6 practical cases, the following is a
       condensed code.
    */
    if (hardMarginXInB + bufferWidthInB < lineWidthInB - hardMarginXInB)
        xLimitInB = hardMarginXInB + bufferWidthInB;
    else
        xLimitInB = lineWidthInB - hardMarginXInB;
    bool theEnd = false;
    uint32_t indexSizeIncrement = bandHeight * lineWidthInB;
    uint32_t localHeight = bandHeight;
    while (pageHeight) {
        std::unique_ptr<Band> current = nullptr;
        bool cmyPlanesHasData = false;
        // Special things to do for the last band
        if (pageHeight < bandHeight) {
            theEnd = true;
            localHeight = pageHeight;
            for (uint32_t i=0; i < page->colorsNr(); i++)
                std::fill(band[i].begin(), band[i].end(), 0);
        }
        for (uint32_t i=0; i < page->colorsNr(); i++) {
            for (uint32_t y=0; y < localHeight; y++) {
                const uint32_t dstRowStart = y * bufferWidthInB;
                const uint32_t srcRowStart = index + y * lineWidthInB;
                const size_t copyWidth = xLimitInB - hardMarginXInB;

                // Copy the actual pixel data
                std::ranges::copy(std::span(planes[i] + srcRowStart + hardMarginXInB, copyWidth),
                                  band[i].begin() + dstRowStart);

                // Clear the rest of the buffer row
                if (bufferWidthInB > copyWidth) {
                    std::ranges::fill(std::span(band[i].data() + dstRowStart + copyWidth,
                                                bufferWidthInB - copyWidth), 0);
                }
            }
        }
        // Are the CMY planes completely empty in the band?
        for (uint32_t i=0; i < static_cast<uint32_t>(page->colorsNr() - 1); i++)
            if (!_isEmptyBand(band[i])) {
                cmyPlanesHasData = true;
                break;
            };
        // Compress the entire band.
        if (cmyPlanesHasData) {
            for (uint32_t i=0; i < page->colorsNr(); i++) {
                auto planeRes = algo->compress(request, band[i],
                                                  bufferWidth, bandHeight);
                if (planeRes) {
                    std::unique_ptr<BandPlane> plane = std::move(*planeRes);
                    plane->setColorNr(static_cast<uint8_t>((1 == page->colorsNr()) ? 4 : i + 1));
                    if (!current)
                        current = std::make_unique<Band>(bandNumber, bufferWidth, bandHeight);
                    current->registerPlane(std::move(plane));
                } else if (planeRes.error() != SP::Error::None) {
                     return SP::Unexpected(planeRes.error());
                }
            }
        } else if (!_isEmptyBand(band[page->colorsNr() - 1])) {
            // Compress only the K band.
            auto planeRes = algo->compress(request,
                                              band[page->colorsNr() - 1],
                                              bufferWidth, bandHeight);
            if (planeRes) {
                std::unique_ptr<BandPlane> plane = std::move(*planeRes);
                plane->setColorNr(4);
                if (!current)
                    current = std::make_unique<Band>(bandNumber, bufferWidth, bandHeight);
                current->registerPlane(std::move(plane));
            } else if (planeRes.error() != SP::Error::None) {
                 return SP::Unexpected(planeRes.error());
            }
        }
        if (current)
            page->registerBand(std::move(current));
        bandNumber++;
        index += indexSizeIncrement;
        pageHeight = theEnd ? 0 : pageHeight - bandHeight;
    }
    if (page->bandsNr() > 0)
        page->setBIH(algo->getBIHdata());
    page->flushPlanes();

    return {};
}
