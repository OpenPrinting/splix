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
