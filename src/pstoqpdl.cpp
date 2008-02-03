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
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


/*
 * Appel des filtres
 * Filter call
 */
static bool _linkFilters(const char *arg1, const char *arg2, const char *arg3,
    const char *arg4, const char *arg5) 
{
    int rasterInput[2], rasterOutput[2];
    int raster, splix;

    // Call pstoraster
    if (pipe(rasterInput) || pipe(rasterOutput)) {
        ERRORMSG(_("Cannot create pipe (%i)"), errno);
        return false;
    }

    // Launch SpliX
    if (!(splix = fork())) {
        // SpliX code

        dup2(rasterOutput[1], STDIN_FILENO);
        close(rasterOutput[0]);
        execl("debug/rastertoqpdl", "rastertoqpdl", arg1, arg2, arg3, arg4, arg5, 
            (char *)NULL);
        ERRORMSG(_("Cannot execute rastertoqpdl (%i)"), errno);
        exit(0);
    }
    DEBUGMSG(_("SpliX launched with PID=%u"), splix);
    
    // Launch the raster
    if (!(raster = fork())) {
        // Raster code
        DEBUGMSG("Hi");
        exit(0);
    }
    DEBUGMSG(_("raster launched with PID=%u"), raster);
    close(rasterInput[1]);

    return true;
}



/*
 * Lecture des fichiers CRD / CSA
 * CSA / CRD read
 */
static char *_readCMSFile(PPDFile& ppd, bool csa)
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

    // Create the real filename
    size = strlen(file) + 30;
    tmp = new char[size];
    if (xResolution)
        snprintf(tmp, size, "%s-%lux%lucms%s", file, xResolution, yResolution,
            csa ? "2" : "");
    else
        snprintf(tmp, size, "%scms%s", file, csa ? "2" : "");

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
    res = new char[fi.st_size];
    if (!fread(res, 1, fi.st_size, handle)) {
        ERRORMSG(_("Cannot read CMS file %s (%i)"), tmp, errno);
        delete[] tmp;
        fclose(handle);
        return NULL;
    }
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
    unsigned long copies;
    char *crd, *csa;
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

    // Open the PPD file
    if (!ppd.open(ppdFile, PPDVERSION, options))
        return 1;

    // Call the other filters
    if (!_linkFilters(argv[1], argv[2], argv[3], argv[4], argv[5])) {
        ERRORMSG(_("Filter error.. Cannot continue"));
        return 1;
    }

    // Get the CRD and CSA information
    crd = _readCMSFile(ppd, false);
    csa = _readCMSFile(ppd, true);
    if (!crd || !csa) {
        DEBUGMSG(_("CMS data are missing. Color correction aborted"));
        if (crd)
            delete[] crd;
        if (csa)
            delete[] csa;
    }




    if (crd)
        delete[] crd;
    if (csa)
        delete[] csa;
    return 0;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

