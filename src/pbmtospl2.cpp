/*
 * 	pbmtospl2.cpp		(C) 2006, Aurélien Croc (AP²C)
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

#include "pbmimage.h"
#include "printer.h"
#include "spl2.h"
#include "error.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cups/ppd.h>
#include <cups/cups.h>

static void _printHelp()
{
	printf(_("Usage: %s [options] <black PBM file> [<cyan PBM "
		"file>] [<magenta PBM file>] [<yellow PBM file>]\n\n"),
		"pbmtospl2");
	printf(_("Available options:\n"));
	printf(_(" -h  --help              Print this help message\n"));
	printf(_(" -o  --output <file>     Change the output file "
				"[default to stdout]\n"));
	printf(_(" -P  --ppd <file>        Select the ppd file to use\n"));
	printf(_(" -p  --papersize <name>  Select the paper to use\n"));
	printf(_(" -r  --resolution <res>  Select the resolution\n"));
}

int main(int argc, const char **_argv)
{
	const char *black=NULL, *cyan=NULL, *magenta=NULL, *yellow=NULL;
	const char *resolution=NULL, *paperSize=NULL;
	const char *output=NULL, *ppdFile=NULL;
	PbmImage *document;
	Printer *printer;
	FILE *ppdHandle;
	ppd_file_t* ppd;
	int i, j, k;
	SPL2 spl2;

const char *argv[] = {"./pbmtospl2", "-P", "/etc/cups/ppd/laser.ppd", "-o", 
	"test.spl2", "/home/aurelien/Desktop/erdre.pbm", NULL };
argc = 7;

	// Check options
	for (i=1; i < argc; i++) {
		if (argv[i][0] != '-')
			break;
		// Long options
		if (argv[i][1] == '-') {
			if (!strcmp(argv[i], "--help")) {
				_printHelp();
				return 0;
			} else if (!strcmp(argv[i], "--output")) {
				output = argv[i+1];
				i++;
			} else if (!strcmp(argv[i], "--ppd")) {
				ppdFile = argv[i+1];
				i++;
			} else if (!strcmp(argv[i], "--resolution")) {
				resolution = argv[i+1];
				i++;
			} else if (!strcmp(argv[i], "--papersize")) {
				paperSize = argv[i+1];
				i++;
			} else {
				fprintf(stderr, _("Invalid argument %s\n\n"),
					argv[i]);
				_printHelp();
				return 1;
			}

		// Short options
		} else {
			k = i+1;
			for (j=1; argv[i][j]; j++) {
				switch (argv[i][j]) {
					case 'h':
						_printHelp();
						return 0;
					case 'o':
						output = argv[k];
						k++;
						break;
					case 'P':
						ppdFile = argv[k];
						k++;
						break;
					case 'r':
						resolution = argv[k];
						k++;
						break;
					case 'p':
						paperSize = argv[k];
						k++;
						break;
					default:
						fprintf(stderr, _("Invalid "
							"argument -%c\n\n"),
							argv[i][j]);
						_printHelp();
						return 1;
				};
			}
			i = k-1;
		}
	}

	// Get the layer files
	if (!argv[i]) {
		fprintf(stderr, _("No black PBM file specified.\n\n"));
		_printHelp();
		return 1;
	} else {
		black = argv[i];
		if (argv[i+1]) {
			cyan = argv[i+1];
			if (argv[i+2]) {
				magenta = argv[i+2];
				if (argv[i+3])
					yellow = argv[i+3];
			}
		}
	}

	// Open the PPD file
	if (ppdFile) {
		if (!(ppdHandle = fopen(ppdFile, "r"))) {
			fprintf(stderr, _("Cannot open PPD file %s\n"), 
				ppdFile);
			return errno;
		}
		ppd = ppdOpen(ppdHandle);
	} else {
		fprintf(stderr, _("No PPD file specified.\n\n"));
		_printHelp();
		return 1;
	}

	// Mark the options
	ppdMarkDefaults(ppd);
	if (resolution)
		ppdMarkOption(ppd, "Resolution", resolution);
	if (paperSize)
		ppdMarkOption(ppd, "PaperSize", paperSize);


	// Prepare the output
	if (output) {
		if (!freopen(output, "w", stdout)) {
			fprintf(stderr, _("Cannot open output file %s\n"), 
				output);
			return errno;
		}
	}
	setbuf(stdout, NULL);


	// Create the document

	// Create the document
	document = new PbmImage(black, magenta, cyan, yellow); 
	if (document->load()) {
		delete document;
		return 1;
	}

	// Create the printer
	printer = new Printer(ppd);

	// Convert and print
	DEBUG("Génération du code....");
	spl2.setPrinter(printer);
	spl2.setOutput(stdout);
	spl2.beginDocument();

	while (!spl2.printPage(document, 1));

	spl2.closeDocument();

	ppdClose(ppd);
	fclose(ppdHandle);
	delete document;
	delete printer;

	return 0;
}

