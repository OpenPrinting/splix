/*
 *      appliargs.cpp           (C) 2007, Aurélien Croc (AP²C)
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
#include "i18n.h"

enum  Errors{
    Internal,
    UnknownOption,
    NotEnoughParameters,
    NotEnoughAloneParameters,
    TooMuchParameters,
};

/*
 * Constructeur - Destructeur
 * Init - Uninit
 */
AppliArgs::AppliArgs()
{
}

AppliArgs::AppliArgs(const QStringList& args)
{
    addSupportedArgs(args);
}


/*
 * Enregistrement des arguments
 * Registers arguments
 */
void AppliArgs::addSupportedArgs(const QString& longArg, 
    const QChar& shortArg, quint8 nrVal)
{
    QString arg = longArg;

    if (arg.at(0) == '~') {
        arg.remove(0, 1);
        _optCanAlterParamsNr << arg;
    }
    _args[longArg] = nrVal;
    if (!shortArg.isNull())
        _shortArgs[shortArg] = arg;
}

void AppliArgs::addSupportedArgs(const QStringList& args)
{
    for (unsigned int i = 0; i < args.size(); i++) {
        const QString arg = args.at(i);
        QString longArg = arg.section(',', 0, 0).section('=', 0, 0);
        quint8 nrVal = arg.section(',', 0, 0).section('=', 1, 1).toUInt();
        const QChar shortArg = arg.section(',', 1, 1).at(0);

        addSupportedArgs(longArg, shortArg, nrVal);
    }
}

bool AppliArgs::parse(int argc, char** argv, quint32 maxParams)
{
    bool canAlterParamsNr = false;
    bool errNotified = false;

    _appliName.clear();
    _values.clear();
    _aloneArgs.clear();
    _errors.clear();

    if (argc <= 0) {
        _errors << QString("%1").arg(Internal);
        return false;
    }

    _appliName = argv[0];

    for (unsigned int i=1; i < argc; i++) {
        unsigned int j = i + 1;
        QString arg = argv[i];

        if (arg.size() > 1 && arg.at(0) == '-') {
            // Long argument
            if (arg.at(1) == '-') {
                QStringList param;
                quint8 nrVal;

                if (!_args.contains(arg.remove(0, 2))) {
                    _errors  << QString("%1:--%2").arg(UnknownOption).
                        arg(arg);
                    continue;
                }
                if (_optCanAlterParamsNr.contains(arg))
                    canAlterParamsNr = true;
                nrVal = _args.value(arg);
                for (; j < argc && nrVal; j++, nrVal--) {
                    QString arg = argv[j];

                    if (arg.at(0) == '-')
                        break;
                    param << arg;
                }
                if (nrVal)
                    _errors  << QString("%1:--%2").arg(NotEnoughParameters).
                        arg(arg);
                else
                    _values[arg] = param;

            // Short argument
            } else {
                for (unsigned int k=1; k < arg.size(); k++) {
                    QStringList param;
                    QString longArg;
                    quint8 nrVal;

                    if (!_shortArgs.contains(arg.at(k))) {
                        _errors << QString("%1:-%2").arg(UnknownOption).
                            arg(arg.at(k));
                        continue;
                    }
                    longArg = _shortArgs.value(arg.at(k));
                    if (_optCanAlterParamsNr.contains(longArg))
                        canAlterParamsNr = true;
                    nrVal = _args.value(longArg);
                    for (; j < argc && nrVal; j++, nrVal--) {
                        QString arg = argv[j];

                        if (arg.at(0) == '-')
                            break;
                        param << arg;
                    }
                    if (nrVal)
                        _errors  << QString("%1:-%2").arg(NotEnoughParameters).
                            arg(arg.at(k));
                    else
                        _values[longArg] = param;
                }
            }

        // Alone argument
        } else {
            if (!maxParams && !errNotified && !canAlterParamsNr) {
                _errors << QString("%1").arg(TooMuchParameters);
                errNotified = true;
            } else if (maxParams) {
                _aloneArgs << arg;
                maxParams--;
            }
        } 

        i = j - 1;
    }

    if (maxParams && !canAlterParamsNr)
        _errors << QString("%1").arg(NotEnoughAloneParameters);

    return _errors.isEmpty() ? true : false;
}

void AppliArgs::printErrors(QTextStream& out) const
{
    for (unsigned int i = 0; i < _errors.size(); i++) {
        QString err = _errors.at(i);
        Errors errNr = (Errors)err.section(':', 0, 0).toUInt();

        switch (errNr) {
            case Internal:
                out << _("Invalid argument number") << endl;
                break;
            case UnknownOption:
                out << QString(_("Unknown option %1")).
                    arg(err.section(':', 1)) << endl;
                break;
            case NotEnoughParameters:
                out << QString(_("Not enough parameter(s) for option %1")).
                    arg(err.section(':', 1)) << endl;
                break;
            case NotEnoughAloneParameters:
                out << QString(_("Not enough parameter(s)")) << endl;
                break;
            case TooMuchParameters:
                out << QString(_("Too much parameter(s)")) << endl;
                break;
            default:
                out << _("Unknown error") << endl;
        };
    }
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8 : */

