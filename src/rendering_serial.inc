
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
