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
#include "cache.h"
#include <stddef.h>
#include <unistd.h>
#include "errlog.h"
#include "semaphore.h"

static unsigned long _pageRequested = 0, _numberOfPages = 0;
static CachePolicy _policy = EveryPagesIncreasing;
static bool _stopCacheControllerThread = false;
static pthread_t _cacheThread;
static Semaphore _work(0);



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
        _work--;

        // Does the thread needs to exit?
        if (*needToExit)
            break;
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
    /** @todo à implémenter */

    return res;
}



/*
 * Enregistrement d'une page dans le cache
 * Register a new page in the cache
 */
void registerPage(Page* page)
{
    /** @todo à implémenter */
}



/*
 * Extraction d'une page du cache
 * Cache page extraction
 */
Page* getPage(unsigned long nr)
{
    /** @todo à implémenter */
    return NULL;
}



/*
 * Modification de la politique de gestion du cache
 * Update the cache policy
 */
void setCachePolicy(CachePolicy policy)
{
    _policy = policy;
}



/*
 * Enregistrer le nombre de pages maximum
 * Set the maximum number of pages
 */
void setNumberOfPages(unsigned long nr)
{
    _numberOfPages = nr;
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

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

