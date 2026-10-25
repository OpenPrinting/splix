/*
 * Part of the SpliX compression implementation.
 * See compress.cpp for copyright and license terms.
 */
static bool _compressBandedPage(const Request& request, Page* page)
{
    unsigned long index=0, pageHeight, pageWidth, lineWidthInB, bandHeight;
    unsigned long bandWidth, bandWidthInB;
    unsigned long bandSize, hardMarginX, hardMarginXInB, hardMarginY;
    unsigned char *planes[4], *band;
    unsigned long bandNumber=0;
    unsigned char colors;

    colors = page->colorsNr();
    hardMarginX = ((unsigned long)ceil(page->convertToXResolution(request.
        printer()->hardMarginX())) + 7) & ~7;
    hardMarginY = ceil(page->convertToYResolution(request.printer()->
        hardMarginY()));
    hardMarginXInB = hardMarginX / 8;
    pageWidth = page->width();
    pageHeight = page->height() - hardMarginY;
    page->setWidth(pageWidth);
    page->setHeight(pageHeight);
    lineWidthInB = (pageWidth + 7) / 8;
    bandHeight = request.printer()->bandHeight();
    if (page->xResolution() == 300 && page->yResolution() == 300)
        bandHeight /= 2;

    //Patch for Samsung M2026
    if (request.printer()->specialBandWidth()) {
        bandWidthInB = _get2020BandWidthInB(request.printer()->paperType(),
                                            page->xResolution(),
                                            pageWidth);
        if (!bandWidthInB) bandWidthInB = lineWidthInB;
    } else if (request.printer()->fixedBandWidth()) {
        //Some of QPDL 1 and 2 models require full-width data
        //regardless of the paper size.
        //4960px (620 bytes) for 600 DPI
        bandWidthInB = splix::fixedBandWidthBytes(page->xResolution());
    }
    else bandWidthInB = lineWidthInB;  //original assumption before M2026 patch
    bandWidth = bandWidthInB * 8;
    bandSize = bandWidthInB * bandHeight;

    index = hardMarginY * lineWidthInB;
    band = new unsigned char[bandSize];
    for (unsigned int i=0; i < colors; i++)
        planes[i] = page->planeBuffer(i);

    /*
     * 1. On vérifier si la bande n'est pas blanche
     *      |-> Si bande blanche, on passe
     *      '-> Sinon, on compresse
     * 2. On rajoute les informations de bande (numéro de bande et de
     *    couleur).
     * 3. On enregistre la bande dans la page.
     * 4. On détruit les buffers de plans dans la page.
     */
    while (pageHeight) {
        unsigned long localHeight = bandHeight;
        Band *current = NULL;
        bool theEnd = false;

        // Special things to do for the last band
        if (pageHeight < bandHeight) {
            theEnd = true;
            localHeight = pageHeight;
            memset(band, 0, bandSize);
        }

        for (unsigned int i=0; i < colors; i++) {
            Algorithm *algo = NULL;
            BandPlane *plane;

            switch (page->compression()) {
                case 0x0D:
                    algo = new Algo0x0D;
                    break;
                case 0x0E:
                    algo = new Algo0x0E;
                    break;
                case 0x11:
                    algo = new Algo0x11;
                    break;
                default:
                    ERRORMSG(_("Unknown compression algorithm. Aborted"));
                    return false;
            }

            const splix::BandLayout layout = algo->reverseLineColumn()
                ? splix::ColumnMajor
                : splix::RowMajor;
            splix::copyBandRows(band, bandWidthInB, bandHeight,
                                planes[i] + index, lineWidthInB,
                                hardMarginXInB, localHeight, layout);

            // Does the band is empty?
            if (_isEmptyBand(band, bandSize)) {
                delete algo;
                continue;
            }

            // Check if bytes have to be reversed
            if (algo->inverseByte())
                for (unsigned int j=0; j < bandSize; j++)
                    band[j] = ~band[j];

            // Call the compression method
            plane = algo->compress(request, band, bandWidth, bandHeight);
            /*
             * If algorithm 0xd did not create a plane, it means that the
             * complementary algorithm 0xE need to be used
             */
            if (!plane && page->compression() == 0x0D) {
                delete algo;
                algo = new Algo0x0E;
                /* Bytes has to be reversed first, as algo0xd didn't do that. */
                for (unsigned int j = 0; j < bandSize; j++)
                    band[j] = ~band[j];
                /* Do the encoding with algo0xe. */
                plane = algo->compress(request, band, bandWidth, bandHeight);
            }

            if (plane) {
                plane->setColorNr(i + 1);
                if (!current)
                    current = new Band(bandNumber, bandWidth, bandHeight);
                current->registerPlane(plane);
            }

            delete algo;
        }
        if (current)
            page->registerBand(current);
        bandNumber++;
        index += lineWidthInB * bandHeight;
        pageHeight = theEnd ? 0 : pageHeight - bandHeight;
    }
    page->flushPlanes();
    delete[] band;

    return true;
}
