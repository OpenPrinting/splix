/*
 * 	    cache.cpp                 (C) 2008, Aurélien Croc (AP²C)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the
 *  Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  $Id$
 * 
 */
#ifndef DISABLE_THREADS
#include <mutex>
#include <condition_variable>
#include <semaphore>
#include <thread>
#include <vector>
#include "sp_semaphore.h"
#include "sp_result.h"

#include "cache.h"
#include <fcntl.h>
#include <stdlib.h>
#include "sp_portable.h"
#include <string.h>
#include <errno.h>
#include "page.h"
#include "errlog.h"
#include <algorithm>

/*
 * Variables internes
 * Internal variables
 */
// Cache controller thread variables
static CachePolicy _policy = EveryPagesIncreasing;
static std::jthread _cacheThread;
static SP::Semaphore _work(0);

// Page request variables
static unsigned long _lastPageRequested = 0, _pageRequested = 0;
static SP::Semaphore _pageAvailable(0);

// Document information
static uint32_t _numberOfPages = 0;
static bool _pageLimitKnown = false;

// Cache variables
static CacheEntry *_waitingList = nullptr, *_lastWaitingList = nullptr;
static std::mutex _waitingListLock;
static uint32_t _pagesInTable = 0;
static std::vector<std::unique_ptr<CacheEntry>> _pages;
static std::mutex _pageTableLock;

// Cache in memory variables
static CacheEntry *_inMemory = nullptr, *_inMemoryLast = nullptr;
static uint32_t _pagesInMemory = 0;



/*
 * Contrôleur de cache (thread)
 * Cache controller (thread)
 */
static void __registerIntoCacheList(CacheEntry *entry, bool swapLast)
{
    unsigned long pageNr;
    CacheEntry *tmp = nullptr;

    if (!_inMemory) {
        entry->setNext(nullptr);
        entry->setPrevious(nullptr);
        _inMemory = entry;
        _inMemoryLast = entry;
        _pagesInMemory = 1;
        return;
    }

    // Locate the previous cache entry
    pageNr = entry->page()->pageNr();
    switch (_policy) {
        case OddIncreasing:
        case EveryPagesIncreasing:
            tmp = _inMemoryLast;
            while (tmp && tmp->page()->pageNr() > pageNr)
                tmp = tmp->previous();
            break;
        case EvenDecreasing:
            if (pageNr % 2) {
                // Odd page
                tmp = _inMemoryLast;
                while (tmp) {
                    if (!(tmp->page()->pageNr() % 2) || tmp->page()->pageNr() < 
                        pageNr)
                        break;
                    tmp = tmp->previous();
                }

            } else {
                // Even page
                tmp = _inMemory;
                while (tmp && !(tmp->page()->pageNr() % 2)) {
                    if (tmp->page()->pageNr() < pageNr)
                        break;
                    tmp = tmp->next();
                }
                tmp = tmp ? tmp->previous() : _inMemoryLast;
            }
            break;
    }

    // Register the new entry
    if (swapLast && tmp == _inMemoryLast) {
        auto res = entry->swapToDisk();
        if (!res) {
            ERRORMSG(_("Failed to swap page %lu to disk: %s"), 
                     static_cast<unsigned long>(entry->page()->pageNr()), 
                     SP::to_string(res.error()).data());
        }
        swapLast = false;
    } else {
        if  (!tmp) {
            entry->setPrevious(NULL);
            entry->setNext(_inMemory);
            _inMemory->setPrevious(entry);
            _inMemory = entry;
        } else {
            entry->setPrevious(tmp);
            entry->setNext(tmp->next());
            if (tmp->next())
                tmp->next()->setPrevious(entry);
            else
                _inMemoryLast = entry;
            tmp->setNext(entry);
        }
        if (!swapLast)
            _pagesInMemory++;
    }

    // Swap to disk the last entry if needed
    if (swapLast) {
        tmp = _inMemoryLast;
        _inMemoryLast = tmp->previous();
        if (tmp->previous())
            tmp->previous()->setNext(nullptr);
        auto res = tmp->swapToDisk();
        if (!res) {
            ERRORMSG(_("Failed to swap page %lu to disk: %s"), 
                     static_cast<unsigned long>(tmp->page()->pageNr()), 
                     SP::to_string(res.error()).data());
        }
    }
}

static void __manageMemoryCache(CacheEntry *entry)
{
    std::lock_guard<std::mutex> lock(_pageTableLock);

    // Append an entry
    if (entry) {
        __registerIntoCacheList(entry, _pagesInMemory >= CACHESIZE);

    // Preload the best page
    } else {
        unsigned int nr;

        if (_inMemoryLast)
            nr = _inMemoryLast->page()->pageNr();
        else
            nr = static_cast<unsigned int>(_lastPageRequested);

        switch (_policy) {
            case EveryPagesIncreasing:
                nr++;
                break;
            case OddIncreasing:
                if (nr % 2)
                    nr += 2;
                else if (nr > 2)
                    nr -= 2;
                else
                    nr = 1;
                break;
            case EvenDecreasing:
                if (nr == 2)
                    nr = 1;
                else
                    nr -= 2;
                break;
        }
        if (nr <= _pages.size() && _pages[nr-1]) {
            if (_pages[nr-1]->isSwapped()) {
                auto res = _pages[nr-1]->restoreIntoMemory();
                if (!res) {
                    ERRORMSG(_("Failed to restore page %u from disk: %s"), 
                             nr, SP::to_string(res.error()).data());
                    _pages[nr-1]->setError(res.error());
                    if (nr == _pageRequested)
                        _pageAvailable.release();
                    return;
                }
            }
            _pages[nr-1]->setNext(nullptr);
            _pages[nr-1]->setPrevious(_inMemoryLast);
            if (_inMemoryLast) {
                _inMemoryLast->setNext(_pages[nr-1].get());
                _inMemoryLast = _pages[nr-1].get();
            } else {
                _inMemory = _pages[nr-1].get();
                _inMemoryLast = _pages[nr-1].get();
            }
            _pagesInMemory++;
            if (nr == _pageRequested)
                _pageAvailable.release();
        }
    }
}static void _cacheControllerThread(std::stop_token stopToken)
{
    bool whatToDo = true;

    DEBUGMSG(_("Cache controller thread loaded and is waiting for a job"));
    while (!stopToken.stop_requested()) {
        bool preloadPage = false;

        // Waiting for a job
        _work.acquire();

#ifdef DUMP_CACHE
        if (_pagesInMemory) {
            CacheEntry *tmp = _inMemory;

            fprintf(stderr, _("DEBUG: \033[34mCache dump: "));
            for (unsigned int i=0; i < _pagesInMemory && tmp; i++) {
                fprintf(stderr, "%lu ", tmp->page()->pageNr());
                tmp = tmp->next();
            }
            fprintf(stderr, "\033[0m\n");
        } else
            fprintf(stderr, _("DEBUG: \033[34mCache empty\033[0m\n"));
#endif /* DUMP_CACHE */

        // Does the thread needs to exit?
        if (stopToken.stop_requested())
            break;


        /*
         * Check what action to do
         */
        // Nothing?
        if (!_waitingList && (_pagesInMemory == CACHESIZE || 
            _pagesInMemory == _pagesInTable))
            continue;
        // new page to append and pages to preload?
        // Choose one action of them to do (and inverse the action to do at 
        // each loop)
        if (_waitingList && !(_pagesInMemory == CACHESIZE || 
            _pagesInMemory == _pagesInTable)) {
            preloadPage = whatToDo;
            whatToDo = !whatToDo;
        // One of the two thing to do
        } else
            preloadPage = (_waitingList == nullptr);

        /*
         * Preload a page
         */
        if (preloadPage) {
            __manageMemoryCache(NULL);

        /*
         * Store a page
         */
        } else {
            CacheEntry *entry;

            // Get the cache entry to store
            {
                std::lock_guard<std::mutex> lock(_waitingListLock);
                entry = _waitingList;
                if (entry) {
                    _waitingList = entry->next();
                    if (_lastWaitingList == entry)
                        _lastWaitingList = nullptr;
                }
            }

            if (!entry)
                continue;

            // Store the entry in the page table
            {
                std::lock_guard<std::mutex> lock(_pageTableLock);

                // Resize the page table if needed
                if (entry->page()->pageNr() > _pages.size()) {
                    _pages.resize(entry->page()->pageNr());
                }

                // Store the page in the table
                _pages[entry->page()->pageNr() - 1] = std::unique_ptr<CacheEntry>(entry);
            }
            _pagesInTable++;

            // Does the main thread needs this page?
            if (_pageRequested == entry->page()->pageNr()) {
                std::lock_guard<std::mutex> lock(_pageTableLock);
                entry->setNext(nullptr);
                entry->setPrevious(nullptr);
                _pageAvailable.release();

            // So check whether the page can be kept in memory or have to
            // be swapped on the disk
            } else 
                __manageMemoryCache(entry);
        }
    }

    DEBUGMSG(_("Cache controller unloaded. See ya"));
}



/*
 * Initialisation et clôture du cache
 * Cache initialization and uninitialization
 */
bool initializeCache()
{
    // Load the cache controller thread
    try {
        _cacheThread = std::jthread(_cacheControllerThread);
    } catch (const std::system_error& e) {
        ERRORMSG(_("Cannot load the cache controller thread: %s"), e.what());
        return false;
    }

    // Initialize the different cache variables
    return true;
}

bool uninitializeCache()
{
    // Stop the cache controller thread
    _cacheThread.request_stop();
    _work.release(); // wake up thread to check for stop
    _cacheThread.join();
 
    // Check if all pages has been read. Otherwise free them
    for (size_t i = 0; i < _pages.size(); i++) {
        if (_pages[i]) {
            ERRORMSG(_("Cache: page %lu hasn't be used!"), static_cast<unsigned long>(i+1));
        }
    }
    _pages.clear();

    return true;
}



/*
 * Enregistrement d'une page dans le cache
 * Register a new page in the cache
 */
void registerPage(std::unique_ptr<Page> page)
{
    auto entry = std::make_unique<CacheEntry>(std::move(page));
    CacheEntry* entryRaw = entry.release();
    {
        std::lock_guard<std::mutex> lock(_waitingListLock);
        if (_lastWaitingList) {
            _lastWaitingList->setNext(entryRaw);
            _lastWaitingList = entryRaw;
        } else {
            _waitingList = entryRaw;
            _lastWaitingList = entryRaw;
        }
    }
    _work.release();
}



/*
 * Extraction d'une page du cache
 * Cache page extraction
 */
SP::Result<std::unique_ptr<Page>> getNextPage()
{
    CacheEntry *entry = nullptr;
    uint32_t nr = 0;
    bool notUnregister = false;

    // Get the next page number
    switch (_policy) {
        case EveryPagesIncreasing:
            nr = static_cast<uint32_t>(_lastPageRequested + 1);
            break;
        case EvenDecreasing:
            if (_lastPageRequested > 2)
                nr = static_cast<uint32_t>(_lastPageRequested - 2);
            else {
                nr = 1;
                setCachePolicy(OddIncreasing);
            }
            break;
        case OddIncreasing:
            if (!_lastPageRequested)
                nr = 1;
            else
                nr = static_cast<uint32_t>(_lastPageRequested + 2);
            break;
    }

    DEBUGMSG(_("Next requested page : %lu (# pages into memory=%lu/%u)"), static_cast<unsigned long>(nr), 
        static_cast<unsigned long>(_pagesInMemory), CACHESIZE);

    // Wait for the page
    while (nr && (!_pageLimitKnown || _numberOfPages >= nr)) {
        {
            std::lock_guard<std::mutex> lock(_pageTableLock);
            if (_pages.size() >= nr && _pages[nr - 1] && 
                !_pages[nr - 1]->isSwapped()) {
                
                entry = _pages[nr - 1].get();
                
                // If the background thread encountered an error, propagate it
                if (entry->error() != SP::Error::None) {
                    return SP::Unexpected(entry->error());
                }

                if (!entry->previous() && !entry->next() && entry != _inMemory)
                    notUnregister = true;
                if (entry->previous())
                    entry->previous()->setNext(entry->next());
                if (entry->next())
                    entry->next()->setPrevious(entry->previous());
                if (entry == _inMemory)
                    _inMemory = entry->next();
                if (entry == _inMemoryLast)
                    _inMemoryLast = entry->previous();
                break;
            } else if (_pages.size() >= nr && _pages[nr - 1] && 
                _pages[nr - 1]->isSwapped())
                _work.release();
            _pageRequested = nr;
        }
        _pageAvailable.acquire();
    }

    // Extract the page instance
    if (!entry)
        return std::unique_ptr<Page>(nullptr);
    
    _pageTableLock.lock();
    auto entryPtr = std::move(_pages[nr - 1]);
    _pageTableLock.unlock();

    _pagesInTable--;
    _lastPageRequested = nr;
    auto page = entryPtr->releasePage();
    // entryPtr (the CacheEntry) is destroyed here when we return or at end of scope

    // Preload a new page
    if (!notUnregister)
        _pagesInMemory--;
    _work.release();

    return page;
}



/*
 * Modification de la politique de gestion du cache
 * Update the cache policy
 */
void setCachePolicy(CachePolicy policy)
{
    _policy = policy;

    // Initialize the lastPageRequested to get the next page number 
    if (policy == EveryPagesIncreasing || policy == OddIncreasing)
        _lastPageRequested = 0;
    else {
        while (!_numberOfPages)
            _pageAvailable.acquire();
        _lastPageRequested = (_numberOfPages & ~0x1) + 2;
    }
}



/*
 * Enregistrer le nombre de pages maximum
 * Set the maximum number of pages
 */
void setNumberOfPages(uint32_t nr)
{
    _numberOfPages = nr;
    _pageLimitKnown = true;
    _pageAvailable.release();
}



/*
 * Gestion des entrées du cache
 * Cache entry management
 */

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

#endif /* DISABLE_THREADS */

std::unique_ptr<Page> CacheEntry::releasePage()
{
    return std::move(_page);
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

