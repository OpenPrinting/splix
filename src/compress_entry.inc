
SP::Result<> compressPage(const Request& request, Page* page)
{
    if (!page)
        return SP::Unexpected(SP::Error::InvalidArgument);

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
            return SP::Unexpected(SP::Error::InvalidArgument);
#endif /* DISABLE_JBIG */
        case 0x15:
#ifndef DISABLE_JBIG
            return _compressBandedJBIGPage(request, page);
#else
            ERRORMSG(_("J-BIG compression algorithm has been disabled during "
                "the compilation. Please recompile SpliX and enable the "
                "J-BIG compression algorithm."));
            return SP::Unexpected(SP::Error::InvalidArgument);
#endif /* DISABLE_JBIG */
        default:
            ERRORMSG(_("Compression algorithm 0x%X does not exist"),
                static_cast<unsigned int>(page->compression()));
            return SP::Unexpected(SP::Error::InvalidArgument);
    }
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */
