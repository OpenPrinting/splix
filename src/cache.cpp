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
 *  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  $Id$
 * 
 */
#ifndef DISABLE_THREADS
#include "cache.h"
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include "page.h"
#include "errlog.h"
#include "semaphore.h"

/*
 * Variables internes
 * Internal variables
 */
// Cache controller thread variables
static CachePolicy _policy = EveryPagesIncreasing;
static bool _stopCacheControllerThread = false;
static pthread_t _cacheThread;
static Semaphore _work(0);

// Page request variables
static unsigned long _lastPageRequested = 0, _pageRequested = 0;
static Semaphore _pageAvailable(0);

// Document information
static unsigned long _numberOfPages = 0;

// Cache variables
static CacheEntry *_waitingList=NULL, *_lastWaitingList=NULL;
static Semaphore _waitingListLock;
static unsigned long _pagesInCache = 0, _pagesInMemory = 0;
static CacheEntry **_pages = NULL;
static Semaphore _pageTableLock;




/*
 * Contrôleur de cache (thread)
 * Cache controller (thread)
 */
static void* _cacheControllerThread(void *_exitVar)
{
    bool *needToExit = (bool *)_exitVar;
    //bool whatToDo = true;

    DEBUGMSG(_("Cache controller thread loaded and is waiting for a job"));
    while (!(*needToExit)) {
        bool preloadPage = false;

        // Waiting for a job
        _work--;

        DEBUGMSG("---Hoplà, un boulot");

        // Does the thread needs to exit?
        if (*needToExit)
            break;

        // Check what to do
        if (!_waitingList)
            continue;
        /** @todo à implémenter */

        // Preload a page
        if (preloadPage) {
            /** @todo à implémenter */

        // Store a page
        } else {
            CacheEntry *entry;

            // Get the cache entry to store
            {
                _waitingListLock.lock();
                entry = _waitingList;
                _waitingList = entry->sibling();
                if (_lastWaitingList == entry)
                    _lastWaitingList = NULL;
                _waitingListLock.unlock();
            }

            // Store the entry in the page table
            {
                _pageTableLock.lock();

                // Resize the page table if needed
                while (entry->page()->pageNr() > _pagesInCache) {
                    if (!_pagesInCache) {
                        _pagesInCache = CACHESIZE;
                        _pages = new CacheEntry*[_pagesInCache];
                        memset(_pages, 0, _pagesInCache * sizeof(CacheEntry*));
                    } else {
                        CacheEntry** tmp = new CacheEntry*[_pagesInCache*10];
                        memcpy(tmp, _pages, _pagesInCache *sizeof(CacheEntry*));
                        memset(tmp + _pagesInCache, 0, _pagesInCache * 9 *
                            sizeof(CacheEntry*));
                        delete[] _pages;
                        _pages = tmp;
                        _pagesInCache *= 10;
                    }
                }

                DEBUGMSG("Page %lu stockée en mémoire",entry->page()->pageNr());
                // Store the page in the table
                _pages[entry->page()->pageNr() - 1] = entry;
                _pageTableLock.unlock();
            }

            // Does the main thread needs this page?
            _pagesInMemory++;
            if (_pageRequested == entry->page()->pageNr())
                _pageAvailable++;

            // So check whether the page can be kept in memory or have to
            // be swapped on the disk
            else 
                _work++;
        }
    }

    DEBUGMSG(_("Cacne controller unloaded. See ya"));
    return NULL;
}



/*
 * Initialisation et clôture du cache
 * Cache initialization and uninitialization
 */
bool initializeCache()
{
    // Load the cache controller thread
    if (pthread_create(&_cacheThread, NULL, _cacheControllerThread, 
        (void *)&_stopCacheControllerThread)) {
        ERRORMSG(_("Cannot load the cache controller thread. Operation "
            "aborted."));
        return false;
    }

    // Initialize the different cache variables
    return true;
}

bool uninitializeCache()
{
    void *threadResult;
    bool res = true;

    // Stop the cache controller thread
    _stopCacheControllerThread = true;
    _work++;
    if (pthread_join(_cacheThread, &threadResult)) {
        ERRORMSG(_("An error occurred while waiting the end of the cache "
            "controller thread"));
        res = false;
    }
 
    // Check if all pages has been read. Otherwise free them
    for (unsigned long i=0; i < _pagesInCache; i++) {
        if (_pages[i]) {
            ERRORMSG(_("Cache: page %lu hasn't be used!"), i+1);
            delete _pages[i]->page();
            delete _pages[i];
        }
    }
    delete[] _pages;

    return res;
}



/*
 * Enregistrement d'une page dans le cache
 * Register a new page in the cache
 */
void registerPage(Page* page)
{
    CacheEntry *entry;
    
    entry = new CacheEntry(page);
    {
        _waitingListLock.lock();
        if (_lastWaitingList) {
            _lastWaitingList->registerSibling(entry);
            _lastWaitingList = entry;
        } else {
            _waitingList = entry;
            _lastWaitingList = entry;
        }
        DEBUGMSG("Page %lu placée en liste d'attente", page->pageNr());
        _waitingListLock.unlock();
    }
    _work++;
}



/*
 * Extraction d'une page du cache
 * Cache page extraction
 */
Page* getNextPage()
{
    CacheEntry *entry = NULL;
    unsigned long nr=0;
    Page *page;

    // Get the next page number
    switch (_policy) {
        case EveryPagesIncreasing:
            nr = _lastPageRequested + 1;
            break;
        case EvenDecreasing:
            if (_lastPageRequested > 2)
                nr = _lastPageRequested - 2;
            break;
        case OddIncreasing:
            if (!_lastPageRequested)
                nr = 1;
            else
                nr = _lastPageRequested + 2;
            break;
    }

    // Wait for the page
    while (nr && (!_numberOfPages || _numberOfPages >= nr)) {
        {
            _pageTableLock.lock();
            if (_pagesInCache > nr && _pages[nr - 1]) {
                entry = _pages[nr - 1];
                _pages[nr - 1] = NULL;
                _pageTableLock.unlock();
                break;
            }
            _pageRequested = nr;
            _pageTableLock.unlock();
        }
        _pageAvailable--;
    };

    // Extract the page instance
    if (!entry)
        return NULL;
    _lastPageRequested = nr;
    page = entry->page();
    delete entry;

    // Preload a new page
    _pagesInMemory--;
    _work++;

    DEBUGMSG("Page %lu extraite du cache", nr);

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
            _pageAvailable--;
        _lastPageRequested = (_numberOfPages & ~0x1) + 2;
    }
}



/*
 * Enregistrer le nombre de pages maximum
 * Set the maximum number of pages
 */
void setNumberOfPages(unsigned long nr)
{
    _numberOfPages = nr;
    _pageAvailable++;
}



/*
 * Gestion des entrées du cache
 * Cache entry management
 */
CacheEntry::CacheEntry(Page* page)
{
    _tempFile = NULL;
    _next = NULL;
    _page = page;
    _swap = 0;
}

CacheEntry::~CacheEntry()
{
    if (_swap) {
        ERRORMSG(_("Destroy a cache entry which is still swapped on disk."));
        unlink(_tempFile);
        delete[] _tempFile;
    }
}

bool CacheEntry::swapToDisk()
{
    /** @todo a implémenter */
    return false;
}

bool CacheEntry::restoreIntoMemory()
{
    /** @todo a implémenter */
    return false;
}

#endif /* DISABLE_THREADS */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

