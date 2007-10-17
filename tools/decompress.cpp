/*
 *      decompress.cpp          (C) 2006-2007, Aurélien Croc (AP²C)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *   $Id$
 */
#include "appliargs.h"
#include <QtCore/QTextStream>
#include <QtCore/QTextCodec>
#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include "qpdl.h"
#include "i18n.h"
#include "pjl.h"

int main(int argc, char** argv)
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("latin1"));
    AppliArgs args(QStringList() << "spl2,2" << "splc,c" << "~help,h" << 
        "~version,v" << "quiet,q" << "decompress,d");
    QTextStream out(stdout), err(stderr);
    bool argsErr, quiet;
    QFile qpdlDocument;


    // Parse arguments
    argsErr = !args.parse(argc, argv, 1);
    if (argsErr || args.isOptionSet("help")) {
        args.printErrors(err);

        out << QString(_("Usage: %1 [options] <QPDL file>")).arg(args.
            applicationName()) << endl;
        out << _("Available options:") << endl;
        out << _("  --decompress, -d          Decompress bands") << endl;
        out << _("  --help, -h                Print this help message") << endl;
        out << _("  --quiet, -q               Be quiet") << endl;
        out << _("  --spl2, -2                This is a SPL2 document "
            "[default]") << endl;
        out << _("  --splc, -c                This is a SPLc document") << endl;
        out << _("  --version, -v             Print the version information") <<
            endl;
        return argsErr ? 1 : 0;
    } else if (args.isOptionSet("version")) {
        out << _("(C) decompress, 2006-2007 by Aurélien Croc") << endl;
        out << _("This project is under the GNU General Public Licence "
            "version 2") << endl;
        out << _("More information => http://splix.ap2c.org") << endl << endl;
        return 0;
    }
    quiet = args.isOptionSet("quiet");

    
    // Open the QPDL document and parse the PJL header
    qpdlDocument.setFileName(args.parameter(0));
    if (!qpdlDocument.open(QIODevice::ReadOnly)) {
        err << QString(_("Error: cannot open file %1 (%2)")).
            arg(qpdlDocument.fileName()).arg(qpdlDocument.error()) << endl;
        return -qpdlDocument.error();
    }
    if (qpdlDocument.read(9) != QByteArray("%-12345X")) {
        err << QString(_("Error: this file is not a QPDL document")) << endl;
        return 1;
    }
    if (!parsePJLHeader(qpdlDocument, quiet, out, err)) {
        err << QString(_("Error: Invalid PJL header")) << endl;
        return 1;
    }

    QPDLDocument document;
    document.setQuiet(quiet);
    document.setDecompressionState(args.isOptionSet("decompress"));
    document.setType(args.isOptionSet("splc") ? QPDLDocument::SPLc :
            QPDLDocument::SPL2);
    document.parse(qpdlDocument, out, err);

    return 0;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8 : */

