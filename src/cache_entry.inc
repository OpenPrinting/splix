CacheEntry::CacheEntry(std::unique_ptr<Page> page) : _page(std::move(page))
{}

CacheEntry::~CacheEntry()
{
    if (!_tempFile.empty()) {
        ERRORMSG(_("Destroy a cache entry which is still swapped on disk."));
        unlink(_tempFile.c_str());
    }
}

SP::Result<> CacheEntry::swapToDisk()
{
    char path[] = "/tmp/splixV2-pageXXXXXX";
    int fd;

    if (!_tempFile.empty()) {
        ERRORMSG(_("Trying to swap a page instance on the disk which is "
            "already swapped."));
        return SP::Unexpected(SP::Error::InvalidState);
    }

    // Create the temporarily file
    if ((fd = mkstemp(path)) == -1) {
        ERRORMSG(_("Cannot swap a page into disk (%i)"), errno);
        return SP::Unexpected(SP::Error::IOError);
    }
    _tempFile = path;

    // Swap the instance into the file
    auto res = _page->swapToDisk(fd);
    if (!_page || !res) {
        unlink(_tempFile.c_str());
        _tempFile.clear();
        ERRORMSG(_("Cannot swap a page into disk"));
        close(fd);
        return res.has_value() ? SP::Unexpected(SP::Error::SerializationError) : res;
    }

    DEBUGMSG(_("Page %lu swapped to disk"), static_cast<unsigned long>(_page->pageNr()));
    close(fd);
    _page.reset();

    return {};
}

SP::Result<> CacheEntry::restoreIntoMemory()
{
    int fd;

    if (_tempFile.empty()) {
        ERRORMSG(_("Trying to restore a page instance into memory which is "
            "already into memory"));
        return SP::Unexpected(SP::Error::InvalidState);
    }

    // Open the swap file
    if ((fd = open(_tempFile.c_str(), O_RDONLY)) == -1) {
        ERRORMSG(_("Cannot restore page into memory (%i)"), errno);
        return SP::Unexpected(SP::Error::IOError);
    }

    // Restore the instance
    auto res = Page::restoreIntoMemory(fd);
    if (!res) {
        ERRORMSG(_("Cannot restore page into memory: %s"), SP::to_string(res.error()).data());
        close(fd);
        return SP::Unexpected(res.error());
    }
    _page = std::move(*res);

    // Destroy the swap file
    close(fd);
    unlink(_tempFile.c_str());
    _tempFile.clear();

    DEBUGMSG(_("Page %lu restored into memory"), static_cast<unsigned long>(_page->pageNr()));
    return {};
}

std::unique_ptr<Page> CacheEntry::releasePage()
{
    return std::move(_page);
}
