/* Page swap-file serialization. See page.cpp for license terms. */
bool Page::swapToDisk(int fd)
{
    unsigned long i;
    Band* band;

    if (_planes[0] || _planes[1] || _planes[2] || _planes[3]) {
        ERRORMSG(_("Cannot swap page instance which still contains bitmap "
            "representation"));
        return false;
    }
    if (!splix::writeAll(fd, &_xResolution, sizeof(_xResolution)) ||
        !splix::writeAll(fd, &_yResolution, sizeof(_yResolution)) ||
        !splix::writeAll(fd, &_width, sizeof(_width)) ||
        !splix::writeAll(fd, &_height, sizeof(_height)) ||
        !splix::writeAll(fd, &_colors, sizeof(_colors)) ||
        !splix::writeAll(fd, &_pageNr, sizeof(_pageNr)) ||
        !splix::writeAll(fd, &_copiesNr, sizeof(_copiesNr)) ||
        !splix::writeAll(fd, &_compression, sizeof(_compression)) ||
        !splix::writeAll(fd, &_empty, sizeof(_empty)) ||
        !splix::writeAll(fd, &_bandsNr, sizeof(_bandsNr)))
        return false;
    if (0x15 == _compression && _bandsNr > 0 && _bih &&
        !splix::writeAll(fd, _bih, 20))
        return false;
    for (i=0, band = _firstBand; i < _bandsNr; i++) {
        if (!band->swapToDisk(fd))
            return false;
        band = band->sibling();
    }
    return true;
}

Page* Page::restoreIntoMemory(int fd)
{
    unsigned long nr;
    Page* page = new Page();
    if (!splix::readAll(fd, &page->_xResolution, sizeof(page->_xResolution)) ||
        !splix::readAll(fd, &page->_yResolution, sizeof(page->_yResolution)) ||
        !splix::readAll(fd, &page->_width, sizeof(page->_width)) ||
        !splix::readAll(fd, &page->_height, sizeof(page->_height)) ||
        !splix::readAll(fd, &page->_colors, sizeof(page->_colors)) ||
        page->_colors > 4 ||
        !splix::readAll(fd, &page->_pageNr, sizeof(page->_pageNr)) ||
        !splix::readAll(fd, &page->_copiesNr, sizeof(page->_copiesNr)) ||
        !splix::readAll(fd, &page->_compression, sizeof(page->_compression)) ||
        !splix::readAll(fd, &page->_empty, sizeof(page->_empty)) ||
        !splix::readAll(fd, &nr, sizeof(nr))) {
        delete page;
        return NULL;
    }
    if (0x15 == page->_compression && nr > 0) {
        unsigned char bih[20];
        if (!splix::readAll(fd, bih, 20)) {
            delete page;
            return NULL;
        }
        page->setBIH(bih);
    }
    for (unsigned int i=0; i < nr; i++) {
        Band *band = Band::restoreIntoMemory(fd);
        if (!band) {
            delete page;
            return NULL;
        }
        page->registerBand(band);
    }
    return page;
}
