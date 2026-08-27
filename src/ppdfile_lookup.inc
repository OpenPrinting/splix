PPDValue PPDFile::get(std::string_view name, std::string_view opt)
{
    const char *valStr = nullptr;
    PPDValue val;

    // 1. Search in job-specific options first
    valStr = cupsGetOption(name.data(), _num_options, _options);

    // 2. Search in destination info (defaults/attributes)
    if (!valStr && _dinfo) {
        if (!opt.empty()) {
            // It might be a choice for a specific option
            ipp_attribute_t *attr = cupsFindDestSupported(CUPS_HTTP_DEFAULT,
                                        _dest.get(), _dinfo.get(), name.data());
            if (attr) {
                // Return default value if present
                valStr = cupsGetOption(name.data(), _dest->num_options, _dest->options);
            }
        } else {
            // Check for general IPP attributes
            valStr = cupsGetOption(name.data(), _dest->num_options, _dest->options);
            if (!valStr) {
                std::string cupsName = std::string("cups-") + std::string(name);
                valStr = cupsGetOption(cupsName.c_str(), _dest->num_options,
                            _dest->options);
            }
        }
    }

    // 3. Fallback to direct PPD file loading
    if (!valStr && _ppd) {
        ppd_choice_t *choice = ppdFindMarkedChoice(_ppd.get(), name.data());
        if (choice) {
            valStr = choice->choice;
        } else {
            ppd_option_t *option = ppdFindOption(_ppd.get(), name.data());
            if (option) {
                valStr = option->defchoice;
            } else {
                // Finally, look for attributes (e.g. *QPDL BandSize)
                ppd_attr_t *attr = nullptr;
                if (!opt.empty()) {
                    attr = ppdFindAttr(_ppd.get(), opt.data(), name.data());
                } else {
                    attr = ppdFindAttr(_ppd.get(), name.data(), nullptr);
                }

                if (attr && attr->value) {
                    valStr = attr->value;
                    // Attributes in PPD often have quotes, e.g., "128"
                    if (valStr[0] == '"') {
                        std::string_view sv(valStr);
                        if (sv.size() >= 2 && sv.back() == '"') {
                            val.set(sv.substr(1, sv.size() - 2));
                            val.setPreformatted();
                            return val;
                        }
                    }
                } else if (!opt.empty() && !name.empty()) {
                    // Fallback: try reversed just in case libcups/PPD is weird
                    attr = ppdFindAttr(_ppd.get(), name.data(), opt.data());
                    if (attr && attr->value) {
                        valStr = attr->value;
                    }
                }

            }
        }
    }

    if (valStr)
        val.set(valStr);

    return val;
}

PPDValue PPDFile::getPageSize(std::string_view name)
{
    PPDValue val;
    cups_size_t size;

    if (_dinfo && cupsGetDestMediaByName(CUPS_HTTP_DEFAULT, _dest.get(), _dinfo.get(), name.data(),
            CUPS_MEDIA_FLAGS_DEFAULT, &size)) {
        // CUPS size is in 100ths of a millimeter. Convert to points (1/72 inch).
        float w = (static_cast<float>(size.width) / 2540.0f) * 72.0f;
        float h = (static_cast<float>(size.length) / 2540.0f) * 72.0f;
        float lm = (static_cast<float>(size.left) / 2540.0f) * 72.0f;
        float bm = (static_cast<float>(size.bottom) / 2540.0f) * 72.0f;
        val.set(w, h, lm, bm);
    } else if (_ppd) {
        // Fallback to direct PPD page size resolution
        ppd_size_t *psize = ppdPageSize(_ppd.get(), name.data());
        if (psize) {
            val.set(psize->width, psize->length, psize->left, psize->bottom);
        }
    }

    return val;
}



/*
 * Gestion des valeurs du PPD
 * PPD values management
 */
