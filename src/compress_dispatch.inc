/*
 * Part of the SpliX compression implementation.
 * See compress.cpp for copyright and license terms.
 */
bool compressPage(const Request& request, Page* page)
{
    switch(page->compression()) {
        case 0x0D:
        case 0x0E:
        case 0x11:
            return _compressBandedPage(request, page);
        case 0x13:
#ifndef DISABLE_JBIG
            return _compressWholePage(request, page);
#else
            ERRORMSG(_("J-BIG compression algorithm has been disabled during "
                "the compilation. Please recompile SpliX and enable the "
                "J-BIG compression algorithm."));
            break;
#endif /* DISABLE_JBIG */
        case 0x15:
#ifndef DISABLE_JBIG
            return _compressBandedJBIGPage(request, page);
#else
            ERRORMSG(_("J-BIG compression algorithm has been disabled during "
                "the compilation. Please recompile SpliX and enable the "
                "J-BIG compression algorithm."));
            break;
#endif /* DISABLE_JBIG */
        default:
            ERRORMSG(_("Compression algorithm 0x%lX does not exist"),
                page->compression());
    }
    return false;
}
