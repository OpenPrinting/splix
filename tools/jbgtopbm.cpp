/*
 *      jbgtopbm.cpp            (C) 2007, Aurélien Croc (AP²C)
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *   $Id$
 */
#include "appliargs.h"
#include <QtCore/QTextStream>
#include <QtCore/QTextCodec>
#include <QtCore/QFile>
#include <QtCore/QByteArray>
#include "i18n.h"
extern "C" {
#	include <jbig.h>
};

static QFile output;

int main(int argc, char** argv)
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("latin1"));
    AppliArgs args(QStringList() << "~help,h" << "~version,v" << "output=1,o");
    QTextStream out(stdout), err(stderr);
    unsigned long int length;
    struct jbg_dec_state sd;
    unsigned char *buffer;
    QString hw("%1 %2\n");
    QByteArray data;
    bool argsErr;
    QFile input;

    // Parse arguments
    argsErr = !args.parse(argc, argv, 1);
    if (argsErr || args.isOptionSet("help")) {
        args.printErrors(err);

        out << QString(_("Usage: %1 [options] <JBIG file>")).arg(args.
            applicationName()) << endl;
        out << _("Available options:") << endl;
        out << _("  --help, -h                Print this help message") << endl;
        out << _("  --output, -o              Select the output file") << endl;
        out << _("  --version, -v             Print the version information") <<
            endl;
        return argsErr ? 1 : 0;
    } else if (args.isOptionSet("version")) {
        out << _("(C) jbgtopbm, 2007 by Aurélien Croc") << endl;
        out << _("This project is under the GNU General Public Licence "
            "version 2") << endl;
        out << _("More information => http://splix.ap2c.org") << endl << endl;
        return 0;
    }


    // Open the input and the output file
    input.setFileName(args.parameter(0));
    if (!input.open(QIODevice::ReadOnly)) {
        err << _("Error: cannot open file ") << args.parameter(0) << endl;
        return -1;
    }
    data = input.readAll();
    buffer = (unsigned char *)data.data();
    length = data.length();
    if (args.isOptionSet("output"))
        output.setFileName(args.optionArg("output", 0));
    else
        output.setFileName("/dev/stdout");
    if (!output.open(QIODevice::WriteOnly)) {
        err << _("Error: cannot open file ") << output.fileName() << endl;
        return -1;
    }


    // Decompress the image
    jbg_dec_init(&sd);
    while (length) {
        unsigned long size;
        int res;

        size = *(unsigned long*)buffer;
        printf("Taille=%lu\n", size);
        buffer += sizeof(size);
        length -= sizeof(size);
        res = jbg_dec_in(&sd, buffer, size, NULL);
        if (res == JBG_EOK) {
            out << _("Processed.") << endl;
            break;
        } else if (res != JBG_EAGAIN) {
            err << _("JBG not ok ") << res << endl;
            break;
        }
        length -= size;
        buffer += size;
    }

    // Store the image
    output.write("P4\n");
    output.write("# Image created by jbgtopbm, (C) 2007 by Aurélien Croc "
        "(AP²C)\n");
    hw = hw.arg(jbg_dec_getwidth(&sd)).arg(jbg_dec_getheight(&sd));
    output.write(hw.toAscii());
    output.write((const char *)jbg_dec_getimage(&sd, 0), jbg_dec_getsize(&sd));
    output.close();

    return 0;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8 : */

