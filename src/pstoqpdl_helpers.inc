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
#include <cstdint>
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
#include <limits>
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

    const std::uintmax_t rawFileSize = fs::file_size(cmsPath);
    if (rawFileSize > std::numeric_limits<size_t>::max()) {
        ERRORMSG(_("CMS file %s is too large"), cmsPath.c_str());
        fclose(handle);
        return "";
    }
    const size_t fileSize = static_cast<size_t>(rawFileSize);
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
