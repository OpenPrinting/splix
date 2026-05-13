/*
 * 	    pstoqpdl.cpp              (C) 2007-2008, Aurélien Croc (AP²C)
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
#include "ppdfile.h"
#include "errlog.h"
#include "version.h"
#include "sp_portable.h"
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(SP_PLATFORM_POSIX)
    #include <sys/wait.h>
#endif
#include <string>
#include <algorithm>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;


/*
 * Appel des filtres
 * Filter call
 */
static std::string _toLower(std::string_view data)
{
    std::string tmp(data);
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), 
        [](unsigned char c){ return std::tolower(c); });
    return tmp;
}

static int _linkFilters(const char *arg1, const char *arg2, const char *arg3,
    const char *arg4, const char *arg5) 
{
    int rasterInput[2], rasterOutput[2];
    int raster, splix;

    // Call pstoraster
    if (pipe(rasterInput) || pipe(rasterOutput)) {
        ERRORMSG(_("Cannot create pipe (%i)"), errno);
        return 0;
    }

    // Launch SpliX
    if (!(splix = fork())) {
        // SpliX code
        close(rasterInput[1]);
        close(rasterInput[0]);
        close(rasterOutput[1]);
        dup2(rasterOutput[0], SP_STDIN_FILENO);
        close(rasterOutput[0]);
        execl(RASTERDIR "/" RASTERTOQPDL, RASTERDIR "/" RASTERTOQPDL, arg1, 
            arg2, arg3, arg4, arg5, static_cast<char*>(nullptr));
        ERRORMSG(_("Cannot execute rastertoqpdl (%i)"), errno);
        _exit(EXIT_FAILURE);
    }
    DEBUGMSG(_("SpliX launched with PID=%u"), splix);
    
    // Launch the raster
    dup2(rasterInput[1], SP_STDOUT_FILENO);
    close(rasterOutput[0]);
    close(rasterInput[1]);
    if (!(raster = fork())) {
        // Raster code
        dup2(rasterInput[0], SP_STDIN_FILENO);
        dup2(rasterOutput[1], SP_STDOUT_FILENO);
        close(rasterInput[0]);
        close(rasterOutput[1]);
        if (access(RASTERDIR "/" GSTORASTER, F_OK) != -1) {
            // gstoraster filter exists
            execl(RASTERDIR "/" GSTORASTER, RASTERDIR "/" GSTORASTER, arg1, arg2, 
                arg3, arg4, arg5, static_cast<char*>(nullptr));
            ERRORMSG(_("Cannot execute gstoraster (%i)"), errno);
        } else {
            // use pstoraster if gstoraster doesn't exist
            execl(RASTERDIR "/" PSTORASTER, RASTERDIR "/" PSTORASTER, arg1, arg2, 
                arg3, arg4, arg5, static_cast<char*>(nullptr));
            ERRORMSG(_("Cannot execute %s (%i)"), PSTORASTER, errno);
        }
        _exit(EXIT_FAILURE);
    }
    DEBUGMSG(_("raster launched with PID=%u"), raster);
    close(rasterInput[0]);
    close(rasterOutput[1]);

    return splix;
}



/*
 * Lecture des fichiers CRD / CSA
 * CSA / CRD read
 */
static std::string _readCMSFile(PPDFile& ppd, const std::string& manufacturer, bool csa)
{
    unsigned long xResolution=0, yResolution=0;
    PPDValue resolution;
    std::string file;

    // Get the base filename
    file = static_cast<const char *>(ppd.get("CMSFile", "General"));
    if (file.empty())
        return "";

    // Get the resolution
    resolution = ppd.get("Resolution");
    if (resolution == "1200dpi")
        xResolution = yResolution = 1200;
    else if (resolution == "600dpi")
        xResolution = yResolution = 600;
    else if (resolution == "1200x600dpi") {
        xResolution = 1200;
        yResolution = 600;
    } else if (resolution == "300dpi") 
        xResolution = yResolution = 300;

    // Get the real filename
    fs::path cmsPath;
    if (xResolution) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s-%lux%lucms%s", file.c_str(), 
            xResolution, yResolution, csa ? "2" : "");
        cmsPath = fs::path(CUPSPROFILE) / manufacturer / buf;
    }
    
    if (cmsPath.empty() || !fs::exists(cmsPath) || !fs::is_regular_file(cmsPath)) {
        cmsPath = fs::path(CUPSPROFILE) / manufacturer / (file + "cms" + (csa ? "2" : ""));
    }

    // Check if it exists, open it and read it
    if (!fs::exists(cmsPath) || !fs::is_regular_file(cmsPath)) {
        ERRORMSG(_("Cannot open CMS file %s (%i)"), cmsPath.c_str(), errno);
        return "";
    }

    FILE *handle = fopen(cmsPath.c_str(), "r");
    if (!handle) {
        ERRORMSG(_("Cannot open CMS file %s (%i)"), cmsPath.c_str(), errno);
        return "";
    }

    size_t fileSize = fs::file_size(cmsPath);
    std::string res;
    res.resize(fileSize);
    if (fread(res.data(), 1, fileSize, handle) != fileSize) {
        ERRORMSG(_("Error while reading CMS file %s"), cmsPath.c_str());
        fclose(handle);
        return "";
    }
    fclose(handle);

    return res;
}




/*
 * PROGRAMME PRINCIPAL
 * MAIN ROUTINE
 */
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

