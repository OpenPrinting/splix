/*
 *      pjl.cpp                 (C) 2007, Aurélien Croc (AP²C)
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
#include "pjl.h"
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include "i18n.h"

bool parsePJLHeader(QFile& qpdlDocument, bool quiet, QTextStream& out, 
    QTextStream& err)
{
    QString line, command, arg;

    if (!quiet)
        out << _("PJL header: ") << endl;

    while (!qpdlDocument.atEnd()) {
        line = qpdlDocument.readLine();
        if (!line.startsWith("@PJL ")) {
            err << QString(_("Unknown PJL argument: %1")).arg(line) << endl;
            return false;
        }
        line.remove(0, 5);
        command = line.section('=', 0, 0).simplified();
        arg = line.section('=', 1).simplified();
        if (command == "ENTER LANGUAGE") {
            if (arg != "QPDL") {
                err << QString(_("Unsupported printer language: %1")).
                    arg(arg) << endl;
                return false;
            }
            if (!quiet)
                out << endl;
            return true;
        }
        if (!quiet)
            out << "    " << command << QString().fill('.', 30 - 
                command.size()) << " = " << arg << endl;
    }
    return false;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8 : */

