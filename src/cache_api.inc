bool initializeCache()
{
    std::lock_guard lock(cacheState.mutex);
    resetCacheLocked();
    cacheState.initialized = true;
    return true;
}

bool uninitializeCache()
{
    std::lock_guard lock(cacheState.mutex);
    for (size_t index = 0; index < cacheState.pages.size(); ++index) {
        if (cacheState.pages[index]) {
            ERRORMSG(_("Cache: page %lu hasn't be used!"),
                     static_cast<unsigned long>(index + 1));
        }
    }
    cacheState.initialized = false;
    resetCacheLocked();
    cacheState.pageAvailable.notify_all();
    return true;
}

void registerPage(std::unique_ptr<Page> page)
{
    if (!page || page->pageNr() == 0) {
        ERRORMSG(_("Cannot register an invalid cache page"));
        return;
    }

    auto entry = std::make_unique<CacheEntry>(std::move(page));
    const size_t pageIndex = entry->page()->pageNr() - 1;
    std::lock_guard lock(cacheState.mutex);
    if (!cacheState.initialized) {
        ERRORMSG(_("Cannot register a page in an uninitialized cache"));
        return;
    }

    if (cacheState.pages.size() <= pageIndex)
        cacheState.pages.resize(pageIndex + 1);
    if (cacheState.pages[pageIndex] &&
        !cacheState.pages[pageIndex]->isSwapped()) {
        --cacheState.residentPages;
    }

    if (cacheState.residentPages >= kCacheCapacity) {
        auto result = entry->swapToDisk();
        if (!result)
            entry->setError(result.error());
    }
    if (!entry->isSwapped())
        ++cacheState.residentPages;
    cacheState.pages[pageIndex] = std::move(entry);
    cacheState.pageAvailable.notify_all();
}

SP::Result<std::unique_ptr<Page>> getNextPage()
{
    std::unique_lock lock(cacheState.mutex);
    if (!cacheState.initialized)
        return SP::Unexpected(SP::Error::InvalidState);

    const uint32_t pageNumber = nextPageNumberLocked();
    cacheState.pageAvailable.wait(lock, [pageNumber] {
        return !cacheState.initialized || hasPageLocked(pageNumber) ||
               (cacheState.pageLimitKnown &&
                pageNumber > cacheState.numberOfPages);
    });
    if (!cacheState.initialized)
        return SP::Unexpected(SP::Error::InvalidState);
    if (!hasPageLocked(pageNumber))
        return std::unique_ptr<Page>(nullptr);

    const size_t pageIndex = pageNumber - 1;
    auto& entry = cacheState.pages[pageIndex];
    if (entry->error() != SP::Error::None)
        return SP::Unexpected(entry->error());

    if (entry->isSwapped()) {
        if (cacheState.residentPages >= kCacheCapacity) {
            auto room = swapResidentPageLocked(pageIndex);
            if (!room)
                return SP::Unexpected(room.error());
        }
        auto restored = entry->restoreIntoMemory();
        if (!restored) {
            entry->setError(restored.error());
            return SP::Unexpected(restored.error());
        }
        ++cacheState.residentPages;
    }

    auto ownedEntry = std::move(entry);
    --cacheState.residentPages;
    cacheState.lastPageRequested = pageNumber;
    return ownedEntry->releasePage();
}

void setCachePolicy(CachePolicy policy)
{
    std::unique_lock lock(cacheState.mutex);
    cacheState.policy = policy;
    if (policy == EveryPagesIncreasing || policy == OddIncreasing) {
        cacheState.lastPageRequested = 0;
        return;
    }

    cacheState.pageAvailable.wait(lock, [] {
        return !cacheState.initialized || cacheState.pageLimitKnown;
    });
    if (cacheState.initialized) {
        cacheState.lastPageRequested =
            (cacheState.numberOfPages & ~uint32_t{1}) + 2;
    }
}

void setNumberOfPages(uint32_t numberOfPages)
{
    std::lock_guard lock(cacheState.mutex);
    cacheState.numberOfPages = numberOfPages;
    cacheState.pageLimitKnown = true;
    cacheState.pageAvailable.notify_all();
}
