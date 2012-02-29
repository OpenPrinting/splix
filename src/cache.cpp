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
#include "cache.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
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
static unsigned long _maxPagesInTable = 0, _pagesInTable = 0;
static CacheEntry **_pages = NULL;
static Semaphore _pageTableLock;

// Cache in memory variables
static CacheEntry *_inMemory = NULL, *_inMemoryLast = NULL;
static unsigned long _pagesInMemory = 0;



/*
 * Contrôleur de cache (thread)
 * Cache controller (thread)
 */
static void __registerIntoCacheList(CacheEntry *entry, bool swapLast)
{
    unsigned long pageNr;
    CacheEntry *tmp=NULL;

    if (!_inMemory) {
        entry->setNext(NULL);
        entry->setPrevious(NULL);
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
        entry->swapToDisk();
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
        tmp->previous()->setNext(NULL);
        tmp->swapToDisk();
    }
}

static void __manageMemoryCache(CacheEntry *entry)
{
    _pageTableLock.lock();

    // Append an entry
    if (entry) {
        __registerIntoCacheList(entry, _pagesInMemory >= CACHESIZE);

    // Preload the best page
    } else {
        unsigned int nr;

        if (_inMemoryLast)
            nr = _inMemoryLast->page()->pageNr();
        else
            nr = _lastPageRequested;

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
        if (nr <= _maxPagesInTable && _pages[nr-1]) {
            if (_pages[nr-1]->isSwapped())
                _pages[nr-1]->restoreIntoMemory();
            _pages[nr-1]->setNext(NULL);
            _pages[nr-1]->setPrevious(_inMemoryLast);
            if (_inMemoryLast) {
                _inMemoryLast->setNext(_pages[nr-1]);
                _inMemoryLast = _pages[nr-1];
            } else {
                _inMemory = _pages[nr-1];
                _inMemoryLast = _pages[nr-1];
            }
            _pagesInMemory++;
            if (nr == _pageRequested)
                _pageAvailable++;
        }
    }
    _pageTableLock.unlock();
}

static void* _cacheControllerThread(void *_exitVar)
{
    bool *needToExit = (bool *)_exitVar;
    bool whatToDo = true;

    DEBUGMSG(_("Cache controller thread loaded and is waiting for a job"));
    while (!(*needToExit)) {
        bool preloadPage = false;

        // Waiting for a job
        _work--;

#ifdef DUMP_CACHE
        if (_pagesInMemory) {
            CacheEntry *tmp = _inMemory;

            fprintf(stderr, _("DEBUG: [34mCache dump: "));
            for (unsigned int i=0; i < _pagesInMemory && tmp; i++) {
                fprintf(stderr, "%lu ", tmp->page()->pageNr());
                tmp = tmp->next();
            }
            fprintf(stderr, "[0m\n");
        } else
            fprintf(stderr, _("DEBUG: [34mCache empty[0m\n"));
#endif /* DUMP_CACHE */

        // Does the thread needs to exit?
        if (*needToExit)
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
            whatToDo = ~whatToDo;
        // One of the two thing to do
        } else
            preloadPage = (_waitingList == NULL);

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
                _waitingListLock.lock();
                entry = _waitingList;
                _waitingList = entry->next();
                if (_lastWaitingList == entry)
                    _lastWaitingList = NULL;
                _waitingListLock.unlock();
            }

            // Store the entry in the page table
            {
                _pageTableLock.lock();

                // Resize the page table if needed
                while (entry->page()->pageNr() > _maxPagesInTable) {
                    if (!_maxPagesInTable) {
                        _maxPagesInTable = CACHESIZE;
                        _pages = new CacheEntry*[_maxPagesInTable];
                        memset(_pages, 0, _maxPagesInTable * 
                            sizeof(CacheEntry*));
                    } else {
                        CacheEntry** tmp = new CacheEntry*[_maxPagesInTable*10];
                        memcpy(tmp, _pages, _maxPagesInTable *
                            sizeof(CacheEntry*));
                        memset(tmp + _maxPagesInTable, 0, _maxPagesInTable * 9 *
                            sizeof(CacheEntry*));
                        delete[] _pages;
                        _pages = tmp;
                        _maxPagesInTable *= 10;
                    }
                }

                // Store the page in the table
                _pages[entry->page()->pageNr() - 1] = entry;
                _pageTableLock.unlock();
            }
            _pagesInTable++;

            // Does the main thread needs this page?
            if (_pageRequested == entry->page()->pageNr()) {
                _pageTableLock.lock();
                entry->setNext(NULL);
                entry->setPrevious(NULL);
                _pageAvailable++;
                _pageTableLock.unlock();

            // So check whether the page can be kept in memory or have to
            // be swapped on the disk
            } else 
                __manageMemoryCache(entry);
        }
    }

    DEBUGMSG(_("Cache controller unloaded. See ya"));
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
    for (unsigned long i=0; i < _maxPagesInTable; i++) {
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
            _lastWaitingList->setNext(entry);
            _lastWaitingList = entry;
        } else {
            _waitingList = entry;
            _lastWaitingList = entry;
        }
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
    bool notUnregister = false;
    Page *page;

    // Get the next page number
    switch (_policy) {
        case EveryPagesIncreasing:
            nr = _lastPageRequested + 1;
            break;
        case EvenDecreasing:
            if (_lastPageRequested > 2)
                nr = _lastPageRequested - 2;
            else {
                nr = 1;
                setCachePolicy(OddIncreasing);
            }
            break;
        case OddIncreasing:
            if (!_lastPageRequested)
                nr = 1;
            else
                nr = _lastPageRequested + 2;
            break;
    }

    DEBUGMSG(_("Next requested page : %lu (# pages into memory=%lu/%u)"), nr, 
        _pagesInMemory, CACHESIZE);

    // Wait for the page
    while (nr && (!_numberOfPages || _numberOfPages >= nr)) {
        {
            _pageTableLock.lock();
            if (_maxPagesInTable >= nr && _pages[nr - 1] && 
                !_pages[nr - 1]->isSwapped()) {
                entry = _pages[nr - 1];
                _pages[nr - 1] = NULL;
                if (!entry->previous() && !entry->next() && entry != _inMemory)
                    notUnregister = true;
                if (entry->previous())
                    entry->previous()->setNext(entry->next());
                if (entry->next())
                    entry->next()->setPrevious(entry->previous());
                if (entry == _inMemory)
                    _inMemory = entry->next();
                if (entry == _inMemoryLast)
                    _inMemoryLast = NULL;
                _pageTableLock.unlock();
                break;
            } else if (_maxPagesInTable >= nr && _pages[nr - 1] && 
                _pages[nr - 1]->isSwapped())
                _work++;
            _pageRequested = nr;
            _pageTableLock.unlock();
        }
        _pageAvailable--;
    };

    // Extract the page instance
    if (!entry)
        return NULL;
    _pagesInTable--;
    _lastPageRequested = nr;
    page = entry->page();
    delete entry;

    // Preload a new page
    if (!notUnregister)
        _pagesInMemory--;
    _work++;

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
}

CacheEntry::~CacheEntry()
{
    if (_tempFile) {
        ERRORMSG(_("Destroy a cache entry which is still swapped on disk."));
        unlink(_tempFile);
        delete[] _tempFile;
    }
}

bool CacheEntry::swapToDisk()
{
    const char *path = "/tmp/splixV2-pageXXXXXX";
    int fd;

    if (_tempFile) {
        ERRORMSG(_("Trying to swap a page instance on the disk which is "
            "already swapped."));
        return false;
    }

    // Create the temporarily file
    _tempFile = new char[strlen(path)+1];
    strcpy(_tempFile, path);
    if ((fd = mkstemp(_tempFile)) == -1) {
        delete[] _tempFile;
        _tempFile = NULL;
        ERRORMSG(_("Cannot swap a page into disk (%i)"), errno);
        return false;
    }

    // Swap the instance into the file
    if (!_page->swapToDisk(fd)) {
        unlink(_tempFile);
        delete[] _tempFile;
        _tempFile = NULL;
        ERRORMSG(_("Cannot swap a page into disk"));
        return false;
    }

    DEBUGMSG(_("Page %lu swapped to disk"), _page->pageNr());
    close(fd);
    delete _page;
    _page = NULL;

    return true;
}

bool CacheEntry::restoreIntoMemory()
{
    int fd;

    if (!_tempFile) {
        ERRORMSG(_("Trying to restore a page instance into memory which is "
            "aready into memory"));
        return false;
    }

    // Open the swap file
    if ((fd = open(_tempFile, O_RDONLY)) == -1) {
        ERRORMSG(_("Cannot restore page into memory (%i)"), errno);
        return false;
    }

    // Restore the instance
    if (!(_page = Page::restoreIntoMemory(fd))) {
        ERRORMSG(_("Cannot restore page into memory"));
        return false;
    }

    // Destroy the swap file
    close(fd);
    unlink(_tempFile);
    delete[] _tempFile;
    _tempFile = NULL;

    DEBUGMSG(_("Page %lu restored into memory"), _page->pageNr());
    return true;
}

#endif /* DISABLE_THREADS */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

