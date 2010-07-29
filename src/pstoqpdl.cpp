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
 *  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  $Id$
 * 
 */
#include "ppdfile.h"
#include "errlog.h"
#include "version.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>


/*
 * Appel des filtres
 * Filter call
 */
static char *_toLower(const char *data)
{
    char *tmp = new char[strlen(data) + 1];
    unsigned int i;

    for (i=0; data[i]; i++)
        tmp[i] = (char) tolower(data[i]);
    tmp[i] = 0;
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
        dup2(rasterOutput[0], STDIN_FILENO);
        close(rasterOutput[0]);
        execl(RASTERDIR "/" RASTERTOQPDL, RASTERDIR "/" RASTERTOQPDL, arg1, 
            arg2, arg3, arg4, arg5, (char *)NULL);
        ERRORMSG(_("Cannot execute rastertoqpdl (%i)"), errno);
        exit(0);
    }
    DEBUGMSG(_("SpliX launched with PID=%u"), splix);
    
    // Launch the raster
    dup2(rasterInput[1], STDOUT_FILENO);
    close(rasterOutput[0]);
    close(rasterInput[1]);
    if (!(raster = fork())) {
        // Raster code
        dup2(rasterInput[0], STDIN_FILENO);
        dup2(rasterOutput[1], STDOUT_FILENO);
        close(rasterInput[0]);
        close(rasterOutput[1]);
        execl(RASTERDIR "/" PSTORASTER, RASTERDIR "/" PSTORASTER, arg1, arg2, 
            arg3, arg4, arg5,(char *)NULL);
        ERRORMSG(_("Cannot execute pstoraster (%i)"), errno);
        exit(0);
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
static char *_readCMSFile(PPDFile& ppd, const char *manufacturer, bool csa)
{
    unsigned long xResolution=0, yResolution=0, size;
    PPDValue resolution;
    const char *file;
    char *tmp, *res;
    struct stat fi;
    FILE *handle;

    // Get the base filename
    file = ppd.get("CMSFile", "General");
    if (!file || !(*file))
        return NULL;

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
    size = strlen(CUPSPPD) + strlen(manufacturer) + strlen(file) + 64;
    tmp = new char[size];
    if (xResolution)
        snprintf(tmp, size, CUPSPPD "/%s/cms/%s-%lux%lucms%s", manufacturer, 
            file, xResolution, yResolution, csa ? "2" : "");
    if (!xResolution || access(tmp, R_OK))
        snprintf(tmp, size, CUPSPPD "/%s/cms/%scms%s", manufacturer, 
            file, csa ? "2" : "");

    // Check if it exists, open it and read it
    if (stat(tmp, &fi) || !(handle = fopen(tmp, "r"))) {
        ERRORMSG(_("Cannot open CMS file %s (%i)"), tmp, errno);
        delete[] tmp;
        return NULL;
    }
    if (!fi.st_size) {
        ERRORMSG(_("CMS file %s is empty"), tmp);
        delete[] tmp;
        fclose(handle);
        return NULL;
    }
    res = new char[fi.st_size + 1];
    if (!fread(res, 1, fi.st_size, handle)) {
        ERRORMSG(_("Cannot read CMS file %s (%i)"), tmp, errno);
        delete[] tmp;
        fclose(handle);
        return NULL;
    }
    res[fi.st_size] = 0;
    fclose(handle);
    delete[] tmp;

    return res;
}



/*
 * PROGRAMME PRINCIPAL
 * MAIN ROUTINE
 */
int main(int argc, char **argv)
{
    const char *jobid, *user, *title, *options, *ppdFile, *file;
    const char *paperType, *manufacturer;
    unsigned long copies;
    bool pageSetup=false;
    char buffer[1024];
    char *crd, *csa;
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
    copies = strtol(argv[4], (char **)NULL, 10);
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
    if (!ppd.open(ppdFile, PPDVERSION, options))
        return 1;
    manufacturer = _toLower(ppd.get("Manufacturer"));
    paperType = ppd.get("MediaType");
    if (!(strcasecmp(paperType, "OFF")))
        paperType = "NORMAL";

    // Call the other filters
    if (!(pid = _linkFilters(argv[1], argv[2], argv[3], argv[4], argv[5]))) {
        ERRORMSG(_("Filter error.. Cannot continue"));
        return 1;
    }

    // Get the CRD and CSA information and send the PostScript data
    crd = _readCMSFile(ppd, manufacturer, false);
    csa = _readCMSFile(ppd, manufacturer, true);
    if (!crd || !csa) {
        WARNMSG(_("CMS data are missing. Color correction aborted"));
        if (crd)
            delete[] crd;
        if (csa)
            delete[] csa;
        while (!(feof(stdin))) {
            fgets((char *)&buffer, sizeof(buffer), stdin);
            fprintf(stdout, "%s", (char *)&buffer); 
        }
    } else {
        // Check for the header
        while (!(feof(stdin))) {
            if (!fgets((char *)&buffer, sizeof(buffer), stdin))
                break;

            // End of the PS header ?
            if (!(memcmp("%%Creator", (char *)&buffer, 9)) ||
                !(memcmp("%%LanguageLevel:", (char *)&buffer, 16))) {
                if (paperType)
                    fprintf(stdout, "/MediaChoice (%s) def\n", paperType);
                fprintf(stdout, "%s", crd);
                fprintf(stdout, "%s", csa);
                fprintf(stdout, "%s", (char *)&buffer); 
                break;

            // End of the header not found?
            } else if (!(memcmp("%%%%BeginPro", (char *)&buffer, 10)) ||
                !(memcmp("%%BeginRes", (char *)&buffer, 10)) ||
                !(memcmp("%%EndComments", (char *)&buffer, 13))) {
                ERRORMSG(_("End of PostScript header not found"));
                fprintf(stdout, "%s", (char *)&buffer); 
                break;
            }
            fprintf(stdout, "%s", (char *)&buffer); 
        }

        // Check for each page
        while (!(feof(stdin))) {
            if (!fgets((char *)&buffer, sizeof(buffer), stdin))
                break;
            if (!(memcmp("%%Page:", (char *)&buffer, 7))) {
                char tmp[sizeof(buffer)];

                if (!fgets((char *)&tmp, sizeof(tmp), stdin)) {
                    fprintf(stdout, "%s", (char *)&buffer);
                    break;
                }
                if (!(memcmp("%%BeginPageSetup", (char *)&tmp, 16)))
                    pageSetup = true;
                else
                    fprintf(stdout, "%s", csa);
                fprintf(stdout, "%s", (char *)&buffer);
                fprintf(stdout, "%s", (char *)&tmp);
            } else if (pageSetup && !(memcmp("%%EndPageSetup", 
                (char *)&buffer, 14))) {
                fprintf(stdout, "%s", (char *)&buffer);
                fprintf(stdout, "%s", csa);
                pageSetup = false;
            } else 
                fprintf(stdout, "%s", (char *)&buffer);
        }
    }


    // Close the output and wait for Splix to be finished
    fclose(stdout);
    waitpid(pid, &err, 0);

    if (crd)
        delete[] crd;
    if (csa)
        delete[] csa;
    return WEXITSTATUS(err);
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

