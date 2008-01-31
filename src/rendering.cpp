/*
 * 	    rendering.cpp             (C) 2006-2008, Aurélien Croc (AP²C)
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
#include "rendering.h"
#include "qpdl.h"
#include "page.h"
#include "cache.h"
#include "errlog.h"
#include "colors.h"
#include "request.h"
#include "printer.h"
#include "compress.h"
#include "document.h"

#ifndef DISABLE_THREADS
#include <pthread.h>
#include "semaphore.h"

static Document document;
static Semaphore _lock;

/*
 * This function is executed by each compression thread
 * It compress each page, page by page and store them
 * into the cache
 */
static void *_compressPage(void* data)
{
    const Request *request = (const Request *)data;
    Page* page;

    do {
        // Load the page
        {
            _lock.lock();
            page = document.getNextRawPage(*request);
            _lock.unlock();
        }
        if (!page) {
            setNumberOfPages(document.numberOfPages());
            break;
        }

        // Apply some colors optimizations
#ifndef DISABLE_BLACKOPTIM
        applyBlackOptimization(page);
#endif /* DISABLE_BLACKOPTIM */

        // Compress the page
        page->setCompression(0x11); // XXX XXX XXX FIXME TO REMOVE
        if (compressPage(*request, page)) {
            DEBUGMSG(_("Page %lu has been compressed and is ready for "
                "rendering"), page->pageNr());

            /** @todo register the page into the cache */

        } else
            ERRORMSG(_("Error while compressing the page. Check the previous "
                "message. Trying to print the other pages."));
    } while (page);

    DEBUGMSG(_("Compression thread: work done. See ya"));

    return NULL;
}

bool render(const Request& request)
{
    pthread_t threads[THREADS];

    // Load the document
    if (!document.load(request)) {
        ERRORMSG(_("Error while rendering the request. Check the previous "
            "message"));
        return false;
    }

    // Load the compression threads
    for (unsigned int i=0; i < THREADS; i++) {
        if (pthread_create(&threads[i], NULL, _compressPage, (void*)&request)) {
            ERRORMSG(_("Cannot load compression threads. Operation aborted."));
            return false;
        }
    }

    /*
     * Ne pas envoyer l'en-tête si duplex en mode manuel : attendre la
     * disponibilité de la dernière page, afin d'éviter le timeout de
     * l'imprimante si le document comporte beaucoup de pages
     */

    // Send the PJL Header
    request.printer()->sendPJLHeader(request);




    DEBUGMSG("J'attend....");










    // Send the PJL footer
    request.printer()->sendPJLFooter(request);

    // Wait for threads to be finished
    for (unsigned int i=0; i < THREADS; i++) {
        void *result;

        if (pthread_join(threads[i], &result))
            ERRORMSG(_("An error occurred while waiting the end of a thread"));
    }

    return true;
}


#else /* DISABLE_THREADS */

bool render(const Request& request)
{
    Document document;
    Page* page;

    // Load the document
    if (!document.load(request)) {
        ERRORMSG(_("Error while rendering the request. Check the previous "
            "message"));
        return false;
    }

    // Send the PJL Header
    request.printer()->sendPJLHeader(request);

    // Send each page
    page = document.getNextRawPage(request);
    while (page) {
#ifndef DISABLE_BLACKOPTIM
        applyBlackOptimization(page);
#endif /* DISABLE_BLACKOPTIM */
        page->setCompression(0x11); // XXX XXX XXX FIXME TO REMOVE
        if (compressPage(request, page)) {
            if (!renderPage(request, page))
                ERRORMSG(_("Error while rendering the page. Check the previous "
                            "message. Trying to print the other pages."));
        } else
            ERRORMSG(_("Error while compressing the page. Check the previous "
                "message. Trying to print the other pages."));
        delete page;
        fprintf(stderr, "PAGE: %lu %lu\n", page->pageNr(), page->copiesNr());
        page = document.getNextRawPage(request);
    }

    // Send the PJL footer
    request.printer()->sendPJLFooter(request);

    return true;
}

#endif /* DISABLE_THREADS */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

