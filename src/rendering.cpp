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
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
#include <thread>
#include <mutex>
#include <vector>

static Document document;
static std::mutex _lock;
static bool _returnState=true;

/*
 * This function is executed by each compression thread
 * It compress each page, page by page and store them
 * into the cache
 */
static void _compressPage(const Request* request)
{
    bool rotateEvenPages;
    rotateEvenPages = request->duplex() == Request::ManualLongEdge;
    do {
        // Load the page
        std::unique_ptr<Page> page;
        {
            std::lock_guard<std::mutex> lock(_lock);
            auto res = document.getNextRawPage(*request);
            if (!res) {
                if (res.error() != SP::Error::EndOfJob) {
                    ERRORMSG(_("Error while reading page from document: %s"), 
                        SP::to_string(res.error()).data());
                    _returnState = false;
                }
                setNumberOfPages(document.numberOfPages());
                break;
            }
            page = std::move(*res);
        }

        // Make rotation on even pages for ManualLongEdge duplex mode
        if (rotateEvenPages && !(page->pageNr() % 2)) {
            page->rotate();
        }

        // Apply some colors optimizations
#ifndef DISABLE_BLACKOPTIM
        applyBlackOptimization(page.get());
#endif /* DISABLE_BLACKOPTIM */

        // Compress the page
        auto res = compressPage(*request, page.get());
        if (res) {
            DEBUGMSG(_("Page %lu has been compressed and is ready for "
                "rendering"), static_cast<unsigned long>(page->pageNr()));
        } else {
            ERRORMSG(_("Error while compressing page %lu: %s. Trying to print the other pages."), 
                static_cast<unsigned long>(page->pageNr()), SP::to_string(res.error()).data());
            page->setEmpty();
            _returnState = false;
        }
        registerPage(std::move(page));
    } while (true);

    DEBUGMSG(_("Compression thread: work done. See ya"));
}

bool render(Request& request)
{
    bool manualDuplex=false, checkLastPage=false, lastPage=false;
    std::vector<std::jthread> threads;
    std::unique_ptr<Page> page;

    // Load the document
    if (!document.load(request)) {
        ERRORMSG(_("Error while rendering the request. Check the previous "
            "message"));
        return false;
    }

    // Load the compression threads
    try {
        for (unsigned int i=0; i < THREADS; i++) {
            threads.emplace_back(_compressPage, &request);
        }
    } catch (const std::system_error& e) {
        ERRORMSG(_("Cannot load compression threads: %s"), e.what());
        return false;
    }

    // Prepare the manual duplex
    if (request.duplex() == Request::ManualLongEdge || 
        request.duplex() == Request::ManualShortEdge) {
            manualDuplex = true;
            setCachePolicy(EvenDecreasing);
    }

    //Load the first page
    /*
     * NOTE: To prevent printer timeout, PJL header must be sent when the first
     * page to render is available (which is very quickly for normal request but
     * can take very long time if a big document in manual duplex is printed).
     */
    {
        auto res = getNextPage();
        if (!res) {
            ERRORMSG(_("Failed to fetch next page from cache: %s"), 
                     SP::to_string(res.error()).data());
            return false;
        }
        page = std::move(*res);
    }

    // Prevent troubles if the last page is an odd page (in manual duplex mode)
    if (manualDuplex && document.numberOfPages() % 2)
        checkLastPage = true;

    // Send the PJL Header
    if (page)
        request.printer()->sendPJLHeader(request, page->compression(),
                             page->xResolution(), page->yResolution() );
    else
        request.printer()->sendPJLHeader(request, 0, 0, 0);

    // Render the whole document
    while (page) {
        if (checkLastPage && document.numberOfPages() == page->pageNr())
            lastPage = true;
        if (!page->isEmpty()) {
            auto res = renderPage(request, page.get(), lastPage);
            if (!res) {
                ERRORMSG(_("Error while rendering the page: %s. Check the previous "
                    "message. Trying to print the other pages."), SP::to_string(res.error()).data());
                _returnState = false;
            }
            fprintf(stderr, "PAGE: %lu %lu\n", static_cast<unsigned long>(page->pageNr()), static_cast<unsigned long>(page->copiesNr()));
        }
        auto res = getNextPage();
        if (!res) {
             ERRORMSG(_("Failed to fetch next page from cache: %s"), 
                      SP::to_string(res.error()).data());
             break;
        }
        page = std::move(*res);
    }

    // Send the PJL footer
    request.printer()->sendPJLFooter(request);

    // Wait for threads to be finished (std::jthread joins automatically, 
    // but we can join explicitly to handle results if needed)
    threads.clear(); 

    return _returnState;
}


#else /* DISABLE_THREADS */

bool render(Request& request)
{
    Document document;
    std::unique_ptr<Page> page;
    bool returnState = true;

    // Load the document
    if (!document.load(request)) {
        ERRORMSG(_("Error while rendering the request. Check the previous "
            "message"));
        return false;
    }

    // Get first Page
    {
        auto res = document.getNextRawPage(request);
        if (res) page = std::move(*res);
        else if (res.error() != SP::Error::EndOfJob) {
             ERRORMSG(_("Error while loading the first page: %s"), SP::to_string(res.error()).data());
             return false;
        }
    }

    // Send the PJL Header
    if (page)
        request.printer()->sendPJLHeader(request, page->compression(),
                             page->xResolution(), page->yResolution() );
    else
        request.printer()->sendPJLHeader(request, 0, 0, 0);

    // Send each page
    while (page) {
#ifndef DISABLE_BLACKOPTIM
        applyBlackOptimization(page.get());
#endif /* DISABLE_BLACKOPTIM */
        auto resComp = compressPage(request, page.get());
        if (resComp) {
            auto resRender = renderPage(request, page.get());
            if (!resRender) {
                ERRORMSG(_("Error while rendering the page: %s. Check the previous "
                            "message. Trying to print the other pages."), SP::to_string(resRender.error()).data());
                returnState = false;
            }
        } else {
            ERRORMSG(_("Error while compressing the page: %s. Trying to print the other pages."),
                SP::to_string(resComp.error()).data());
            returnState = false;
        }
        fprintf(stderr, "PAGE: %lu %lu\n", static_cast<unsigned long>(page->pageNr()), static_cast<unsigned long>(page->copiesNr()));
        auto resPrev = document.getNextRawPage(request);
        if (!resPrev) {
            if (resPrev.error() != SP::Error::EndOfJob) {
                ERRORMSG(_("Error while loading next page: %s"), SP::to_string(resPrev.error()).data());
            }
            break;
        }
        page = std::move(*resPrev);
    }

    // Send the PJL footer
    request.printer()->sendPJLFooter(request);

    return returnState;
}

#endif /* DISABLE_THREADS */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

