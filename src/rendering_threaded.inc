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
