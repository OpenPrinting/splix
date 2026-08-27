static SP::Result<> _compressBandedPage(const Request& request, Page* page)
{
    uint32_t index=0, pageHeight, pageWidth, lineWidthInB, bandHeight;
    uint32_t bandWidth, bandWidthInB, bandSize;
    uint32_t hardMarginX, hardMarginXInB, hardMarginY;
    const uint8_t *planes[4];
    uint32_t bandNumber=0;
    uint8_t colors;

    if (!request.printer()) {
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    colors = page->colorsNr();
    hardMarginX = (static_cast<uint32_t>(std::ceil(page->convertToXResolution(request.
        printer()->hardMarginX()))) + 7) & ~7;
    hardMarginY = static_cast<uint32_t>(std::ceil(page->convertToYResolution(request.printer()->
        hardMarginY())));
    hardMarginXInB = hardMarginX / 8;
    pageWidth = page->width();
    pageHeight = page->height();

    if (pageHeight <= hardMarginY) {
         ERRORMSG(_("Page height (%u) is smaller than hard margin (%u)"), pageHeight, hardMarginY);
         return SP::Unexpected(SP::Error::InvalidArgument);
    }
    pageHeight -= hardMarginY;

    page->setWidth(pageWidth);
    page->setHeight(pageHeight);
    lineWidthInB = (pageWidth + 7) / 8;
    bandHeight = static_cast<uint32_t>(request.printer()->bandHeight());
    if (page->xResolution() == 300 && page->yResolution() == 300)
        bandHeight /= 2;

    if (!bandHeight) {
        ERRORMSG(_("Invalid zero band height"));
        return SP::Unexpected(SP::Error::InvalidArgument);
    }

    //Patch for Samsung M2026
    if (request.printer()->specialBandWidth()) {
        bandWidthInB = static_cast<uint32_t>(_get2020BandWidthInB(request.printer()->paperType(),
                                            page->xResolution(),
                                            pageWidth));
        if (!bandWidthInB) bandWidthInB = lineWidthInB;
    } else if (request.printer()->fixedBandWidth()) {
        //Some of QPDL 1 and 2 models require full-width data
        //regardless of the paper size.
        //4960px (620 bytes) for 600 DPI
        bandWidthInB = page->xResolution() * 4960 / 600 / 8;
    }
    else bandWidthInB = lineWidthInB;  //original assumption before M2026 patch
    bandWidth = bandWidthInB * 8;

    // Safety check for bandSize
    if (static_cast<uint64_t>(bandWidthInB) * bandHeight > 0x20000000) { // 512MB limit
        ERRORMSG(_("Calculated band size is too large"));
        return SP::Unexpected(SP::Error::MemoryError);
    }
    bandSize = bandWidthInB * bandHeight;

    index = hardMarginY * lineWidthInB;

    std::vector<uint8_t> band;
    try {
        band.resize(bandSize);
    } catch (const std::bad_alloc&) {
        return SP::Unexpected(SP::Error::MemoryError);
    }

    for (uint32_t i=0; i < colors; i++) {
        planes[i] = page->planeBuffer(static_cast<uint8_t>(i));
        if (!planes[i]) {
            ERRORMSG(_("Plane %u has no data"), i);
            return SP::Unexpected(SP::Error::LogicError);
        }
    }

    while (pageHeight) {
        uint32_t localHeight = bandHeight;
        std::unique_ptr<Band> current = nullptr;
        bool theEnd = false;

        // Special things to do for the last band
        if (pageHeight < bandHeight) {
            theEnd = true;
            localHeight = pageHeight;
            std::fill(band.begin(), band.end(), 0);
        }

        for (uint32_t i=0; i < colors; i++) {
            std::unique_ptr<Algorithm> algo = nullptr;
            SP::Result<std::unique_ptr<BandPlane>> planeRes = SP::Unexpected(SP::Error::None);

            switch (page->compression()) {
                case 0x0D:
                    algo = std::make_unique<Algo0x0D>();
                    break;
                case 0x0E:
                    algo = std::make_unique<Algo0x0E>();
                    break;
                case 0x11:
                    algo = std::make_unique<Algo0x11>();
                    break;
                default:
                    ERRORMSG(_("Unknown compression algorithm. Aborted"));
                    return SP::Unexpected(SP::Error::InvalidArgument);
            }

            // Copy the data into the band depending on the algorithm options
            if (algo->reverseLineColumn()) {
                // Special case where columns and lines are swapped (transposed)
                for (uint32_t y = 0; y < localHeight; y++) {
                    const uint32_t srcRowStart = index + hardMarginXInB + y * lineWidthInB;
                    const size_t copyWidth = std::min(static_cast<size_t>(lineWidthInB - hardMarginXInB),
                                                   static_cast<size_t>(bandWidthInB));

                    for (uint32_t x = 0; x < copyWidth; x++) {
                        band[x * bandHeight + y] = planes[i][srcRowStart + x];
                    }
                    // Filling remaining with 0 if necessary
                    for (uint32_t x = static_cast<uint32_t>(copyWidth); x < bandWidthInB; x++) {
                        band[x * bandHeight + y] = 0;
                    }
                }
            } else {
                // Standard row-wise copy
                for (uint32_t y = 0; y < localHeight; y++) {
                    const uint32_t dstRowStart = y * bandWidthInB;
                    const uint32_t srcRowStart = index + hardMarginXInB + y * lineWidthInB;
                    const size_t copyWidth = std::min(static_cast<size_t>(lineWidthInB - hardMarginXInB),
                                                   static_cast<size_t>(bandWidthInB));

                    std::ranges::copy(std::span(planes[i] + srcRowStart, copyWidth), band.begin() + dstRowStart);
                    if (bandWidthInB > copyWidth) {
                        std::ranges::fill(std::span(band.data() + dstRowStart + copyWidth, bandWidthInB - copyWidth), 0);
                    }
                }
            }

            // Is the band empty?
            if (_isEmptyBand(band)) {
                continue;
            }

            // Check if bytes have to be reversed
            if (algo->inverseByte())
                for (uint32_t j=0; j < bandSize; j++)
                    band[j] = ~band[j];

            // Call the compression method
            planeRes = algo->compress(request, band, bandWidth, bandHeight);
            /*
             * If algorithm 0x0D did not create a plane, it means that the
             * complementary algorithm 0x0E needs to be used
             */
            if (!planeRes && page->compression() == 0x0D) {
                algo = std::make_unique<Algo0x0E>();
                /* Bytes must be reversed first, as algo0x0D didn't do that. */
                for (uint32_t j = 0; j < bandSize; j++)
                    band[j] = ~band[j];
                /* Do the encoding with algo0x0E. */
                planeRes = algo->compress(request, band, bandWidth, bandHeight);
            }

            if (planeRes) {
                std::unique_ptr<BandPlane> plane = std::move(*planeRes);
                plane->setColorNr(static_cast<uint8_t>(i + 1));
                if (!current)
                    current = std::make_unique<Band>(bandNumber, bandWidth, bandHeight);
                current->registerPlane(std::move(plane));
            } else if (planeRes.error() != SP::Error::None) {
                 return SP::Unexpected(planeRes.error());
            }
        }
        if (current)
            page->registerBand(std::move(current));
        bandNumber++;
        index += lineWidthInB * bandHeight;
        pageHeight = theEnd ? 0 : pageHeight - bandHeight;
    }
    page->flushPlanes();

    return {};
}
