/*
 * Part of the SpliX compression implementation.
 * See compress.cpp for copyright and license terms.
 */
static bool _compressBandedJBIGPage(const Request& request, Page* page)
{
    unsigned long index=0, pageHeight, lineWidthInB, bandHeight = 128;
    unsigned long bufferWidth, specialBufferWidthInB, bandSize, hardMarginXInB=13, hardMarginY=100;
    unsigned char *planes[4], *band[4];
    unsigned long bandNumber=0, xLimitInB, bufferWidthInB;
    Algo0x15 *algo = new Algo0x15;
    /* Image trimming are done from hardware margins defined in the ppd. */
    hardMarginXInB = ((unsigned long)ceil(page->convertToXResolution(request.
        printer()->hardMarginX())) + 7) / 8;
    hardMarginY = ceil(page->convertToYResolution(request.printer()->
        hardMarginY()));
    bandHeight = request.printer()->bandHeight();
    lineWidthInB = ((page->width()) + 7) / 8;
    // Compute an updated page height, clipped on top.
    pageHeight = page->height() - hardMarginY;
    // Compute the buffer width that is nearest multiple of 256 to the original.
    bufferWidth = page->width() & ~255;
    if ((bufferWidth + 128) < page->width())
      bufferWidth += 256;

    if (request.printer()->specialBandWidth()) {
        specialBufferWidthInB = _get2020BandWidthInB(request.printer()->paperType(),
                                                page->xResolution(),
                                                page->width());
        if (specialBufferWidthInB) bufferWidth = specialBufferWidthInB * 8;
    }
    // Update the page heigth.
    page->setHeight(pageHeight);
    // Update the page width.
    page->setWidth(bufferWidth);
    bufferWidthInB = (bufferWidth + 7) / 8;
    bandSize = bufferWidthInB * bandHeight;
    index = hardMarginY * lineWidthInB;
    for (unsigned int i=0; i < page->colorsNr(); i++) {
        band[i] = new unsigned char[bandSize];
        planes[i] = page->planeBuffer(i);
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
    unsigned long indexSizeIncrement = bandHeight * lineWidthInB;
    unsigned long localHeight = bandHeight;
    while (pageHeight) {
        Band *current = NULL;
        bool cmyPlanesHasData = false;
        // Special things to do for the last band
        if (pageHeight < bandHeight) {
            theEnd = true;
            localHeight = pageHeight;
            for (unsigned int i=0; i < page->colorsNr(); i++)
                memset(band[i], 0, bandSize);
        }
        for (unsigned int i=0; i < page->colorsNr(); i++) {
            for (unsigned int y=0; y < localHeight; y++) {
                for (unsigned int x=hardMarginXInB; x < xLimitInB; x++)
                    band[i][x - hardMarginXInB + y * bufferWidthInB] =
                             planes[i][index + x + y * lineWidthInB];
                for (unsigned int x=lineWidthInB - 2 * hardMarginXInB;
                                  x < bufferWidthInB; x++)
                    band[i][x + y * bufferWidthInB] = 0;
            }
        }
        // Are the CMY planes completely empty in the band?
        for (unsigned int i=0;
             i + 1 < static_cast<unsigned int>(page->colorsNr()); i++)
            if (!_isEmptyBand(band[i], bandSize)) {
                cmyPlanesHasData = true;
                break;
            };
        // Compress the entire band.
        if (cmyPlanesHasData) {
            for (unsigned int i=0; i < page->colorsNr(); i++) {
                BandPlane *plane = algo->compress(request, band[i],
                                                  bufferWidth, bandHeight);
                if (plane) {
                    plane->setColorNr((1 == page->colorsNr()) ? 4:i + 1);
                    if (!current)
                        current = new Band(bandNumber, bufferWidth, bandHeight);
                    current->registerPlane(plane);
                }
            }
        } else if (!_isEmptyBand(band[page->colorsNr() - 1], bandSize)) {
            // Compress only the K band.
            BandPlane *plane = algo->compress(request,
                                              band[page->colorsNr() - 1],
                                              bufferWidth, bandHeight);
            if (plane) {
                plane->setColorNr(4);
                if (!current)
                    current = new Band(bandNumber, bufferWidth, bandHeight);
                current->registerPlane(plane);
            }
        }
        if (current)
            page->registerBand(current);
        bandNumber++;
        index += indexSizeIncrement;
        pageHeight = theEnd ? 0 : pageHeight - bandHeight;
    }
    if (page->bandsNr() > 0)
        page->setBIH(algo->getBIHdata());
    page->flushPlanes();
    for (unsigned int i=0; i < page->colorsNr(); i++)
        delete[] band[i];
    delete algo;
    return true;
}

static bool _compressWholePage(const Request& request, Page* page)
{
    unsigned long hardMarginX, hardMarginXInB, hardMarginY, lineWidthInB;
    unsigned long pageWidth, bandHeight, planeHeight, pageHeight, index;
    unsigned long bandNumber=0;
    unsigned char *buffer;
    Band *current = NULL;
    Algo0x13 algo[4];

    hardMarginX = ((unsigned long)ceil(page->convertToXResolution(request.
        printer()->hardMarginX())) + 7) & ~7;
    hardMarginY = ceil(page->convertToYResolution(request.printer()->
        hardMarginY()));
    hardMarginXInB = hardMarginX / 8;
    pageWidth = page->width() - hardMarginX * 2;
    pageHeight = page->height() - hardMarginY * 2;
    page->setWidth(pageWidth);
    page->setHeight(pageHeight);
    lineWidthInB = (pageWidth + 7) / 8;
    bandHeight = request.printer()->bandHeight();
    // Alignment of the page height on band height
    planeHeight = ((pageHeight + bandHeight - 1) / bandHeight) * bandHeight;
    buffer = new unsigned char[lineWidthInB * planeHeight];

    do {
        current = NULL;
        for (unsigned int i=0; i < page->colorsNr(); i++) {
            unsigned char *curPlane = page->planeBuffer(i);
            BandPlane *plane;

            index = hardMarginY * (lineWidthInB + 2 * hardMarginXInB) +
                hardMarginXInB;
            for (unsigned int y=0; y < pageHeight; y++,
                    index += 2 * hardMarginXInB) {
                for (unsigned int x=0; x < lineWidthInB; x++, index++) {
                    buffer[x + y * lineWidthInB] = curPlane[index];
                }
            }
            memset(buffer + pageHeight * lineWidthInB, 0,
                   (planeHeight - pageHeight) * lineWidthInB);

            // Call the compression method
            plane = algo[i].compress(request, buffer, page->width(),
                planeHeight);
            if (plane) {
                plane->setColorNr(i + 1);
                if (!current)
                    current = new Band(bandNumber, page->width(),
                        request.printer()->bandHeight());
                current->registerPlane(plane);
            }
        }
        if (current)
            page->registerBand(current);
        bandNumber++;
    } while (current);

    page->flushPlanes();
    delete[] buffer;

    return true;
}
