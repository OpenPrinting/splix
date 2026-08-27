int main(int argc, char **argv)
{
    [[maybe_unused]] const char *jobid;
    [[maybe_unused]] const char *user;
    [[maybe_unused]] const char *title;
    [[maybe_unused]] const char *options;
    const char *ppdFile;
    const char *file;
    const char *paperType;
    std::string manufacturer;
    [[maybe_unused]] unsigned long copies;
    bool pageSetup=false;
    char buffer[1024];
    std::string crd, csa;
    int pid, err;
    PPDFile ppd;

    // Check the given arguments
    if (argc != 6 && argc != 7) {
        fprintf(stderr, _("Usage: %s job-id user title copies options "
            "[file]\n"), argv[0]);
        return 1;
    }
    jobid = argv[1];
    user = argv[2];
    title = argv[3];
    options = argv[5];
    file = argc == 7 ? argv[6] : NULL;
    copies = strtol(argv[4], nullptr, 10);
    ppdFile = getenv("PPD");

    // Get more information on the SpliX environment (for debugging)
    DEBUGMSG(_("PS => SpliX filter V. %s by Aurélien Croc (AP²C)"), VERSION);
    DEBUGMSG(_("More information at: http://splix.ap2c.org"));

    // Open the given file
    if (file && !freopen(file, "r", stdin)) {
        ERRORMSG(_("Cannot open file %s"), file);
        return errno;
    }

    // Open the PPD file and get paper information
    if (auto res = ppd.open(ppdFile ? ppdFile : "", PPDVERSION, options); !res) {
        ERRORMSG(_("Failed to open PPD file: %s"), SP::to_string(res.error()).data());
        return 1;
    }
    manufacturer = _toLower(static_cast<const char *>(ppd.get("Manufacturer")));
    paperType = ppd.get("MediaType");
    auto compare = [](std::string_view a, std::string_view b) {
        return std::ranges::equal(a, b, [](char c1, char c2) {
            return std::tolower(static_cast<unsigned char>(c1)) ==
                   std::tolower(static_cast<unsigned char>(c2));
        });
    };

    if (paperType && compare(paperType, "OFF"))
        paperType = "NORMAL";

    // Call the other filters
    if (!(pid = _linkFilters(argv[1], argv[2], argv[3], argv[4], argv[5]))) {
        ERRORMSG(_("Filter error.. Cannot continue"));
        return 1;
    }

    // Get the CRD and CSA information and send the PostScript data
    crd = _readCMSFile(ppd, manufacturer, false);
    csa = _readCMSFile(ppd, manufacturer, true);
    if (crd.empty() || csa.empty()) {
        WARNMSG(_("CMS data are missing. Color correction aborted"));
        while (!(feof(stdin))) {
            if (!fgets(buffer, sizeof(buffer), stdin))
                break;
            fprintf(stdout, "%s", buffer);
        }
    } else {

        // Insert the MediaChoice and colour correction information into
        // the postscript header.
        //
        // Look for a "%%Creator" line in the postscript header, and
        // insert the information before it.
        //
        // Postscript that is created by pdftops from content from Apple
        // iOS devices (iPad etc) seems not to have a "%%Creator" line in
        // the header, so we insert a dummy one. Without this, pstoqpdl
        // seems to crash ghostscript.
        //
        // NB: according to the PostScript Document Structuring Conventions
        // (DSC) specification the end of the postscript header should be
        // the "%%EndComments" line - see:
        // http://partners.adobe.com/public/developer/en/ps/5001.DSC_Spec.pdf


        // search each line in the postscript header
        while (!(feof(stdin))) {

            // read a line of the input file
            if (!fgets(buffer, sizeof(buffer), stdin))
                break;

            if (!(memcmp("%%Creator", buffer, 9)) ||
                !(memcmp("%%LanguageLevel:", buffer, 16))) {
                // found a "%%Creator" line

                // emit the MediaChoice and colour correction information
                if (paperType)
                    fprintf(stdout, "/MediaChoice (%s) def\n", paperType);
                fprintf(stdout, "%s", crd.c_str());
                fprintf(stdout, "%s", csa.c_str());

                // emit the original "%%Creator" line
                fprintf(stdout, "%s", buffer);

                // stop scanning the header
                break;
            }


            if (!(memcmp("%%EndComments", buffer, 13))) {
                // reached end of header without finding a "%%Creator" line

                // emit the MediaChoice and colour correction information
                if (paperType)
                    fprintf(stdout, "/MediaChoice (%s) def\n", paperType);
                fprintf(stdout, "%s", crd.c_str());
                fprintf(stdout, "%s", csa.c_str());

                // emit a dummy "%%Creator" line
                DEBUGMSG(_("inserting dummy \"Creator\" entry in postscript header"));
                fprintf(stdout, "%s", "%%Creator: SpliX pstoqpdl filter");

                // emit the original "%%EndComments" line
                fprintf(stdout, "%s", buffer);

                // stop scanning the header
                break;
            }


            if (!(memcmp("%%BeginPro", buffer, 10)) ||
                !(memcmp("%%BeginRes", buffer, 10))) {
                // we shouldn't find either of these lines in the header

                ERRORMSG(_("End of PostScript header not found"));

                // emit the line that was found
                fprintf(stdout, "%s", buffer);

                // stop scanning the header
                break;
            }

            // encountered some other kind of header line - just emit it
            fprintf(stdout, "%s", buffer);
        }


        // Check for each page
        while (!(feof(stdin))) {
            if (!fgets(buffer, sizeof(buffer), stdin))
                break;
            if (!(memcmp("%%Page:", buffer, 7))) {
                char tmp[sizeof(buffer)];

                if (!fgets(tmp, sizeof(tmp), stdin)) {
                    fprintf(stdout, "%s", buffer);
                    break;
                }
                if (!(memcmp("%%BeginPageSetup", tmp, 16)))
                    pageSetup = true;
                else
                    fprintf(stdout, "%s", csa.c_str());
                fprintf(stdout, "%s", buffer);
                fprintf(stdout, "%s", tmp);
            } else if (pageSetup && !(memcmp("%%EndPageSetup",
                buffer, 14))) {
                fprintf(stdout, "%s", buffer);
                fprintf(stdout, "%s", csa.c_str());
                pageSetup = false;
            } else
                fprintf(stdout, "%s", buffer);
        }
    }


    // Close the output and wait for Splix to be finished
    fclose(stdout);
    waitpid(pid, &err, 0);

    return WEXITSTATUS(err);
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */
