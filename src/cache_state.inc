#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <vector>

#include "cache.h"
#include "errlog.h"
#include "page.h"
#include "sp_portable.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

namespace {
constexpr size_t kCacheCapacity = CACHESIZE > 0 ? CACHESIZE : 1;

struct CacheState {
    std::mutex mutex;
    std::condition_variable pageAvailable;
    CachePolicy policy = EveryPagesIncreasing;
    uint32_t lastPageRequested = 0;
    uint32_t numberOfPages = 0;
    bool pageLimitKnown = false;
    bool initialized = false;
    size_t residentPages = 0;
    std::vector<std::unique_ptr<CacheEntry>> pages;
};

CacheState cacheState;

bool hasPageLocked(uint32_t pageNumber)
{
    return pageNumber > 0 && pageNumber <= cacheState.pages.size() &&
           cacheState.pages[pageNumber - 1] != nullptr;
}

uint32_t nextPageNumberLocked()
{
    switch (cacheState.policy) {
        case EveryPagesIncreasing:
            return cacheState.lastPageRequested + 1;
        case EvenDecreasing:
            if (cacheState.lastPageRequested > 2)
                return cacheState.lastPageRequested - 2;
            cacheState.policy = OddIncreasing;
            return 1;
        case OddIncreasing:
            return cacheState.lastPageRequested == 0
                ? 1 : cacheState.lastPageRequested + 2;
    }
    return 0;
}

SP::Result<> swapResidentPageLocked(size_t excludedIndex)
{
    for (size_t index = cacheState.pages.size(); index > 0; --index) {
        const size_t candidateIndex = index - 1;
        auto& candidate = cacheState.pages[candidateIndex];
        if (candidateIndex == excludedIndex || !candidate ||
            candidate->isSwapped()) {
            continue;
        }

        auto result = candidate->swapToDisk();
        if (!result) {
            candidate->setError(result.error());
            return result;
        }
        --cacheState.residentPages;
        return {};
    }
    return SP::Unexpected(SP::Error::InvalidState);
}

void resetCacheLocked()
{
    cacheState.pages.clear();
    cacheState.policy = EveryPagesIncreasing;
    cacheState.lastPageRequested = 0;
    cacheState.numberOfPages = 0;
    cacheState.pageLimitKnown = false;
    cacheState.residentPages = 0;
}
}  // namespace
