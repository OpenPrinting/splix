/*
 *      appliargs.h             (C) 2006-2007, Aurélien Croc (AP²C)
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
#ifndef _APPLIARGS_H_
#define _APPLIARGS_H_

#include <QtCore/QHash>
#include <QtCore/QChar>
#include <QtCore/QString>
#include <QtCore/QStringList>


class QTextStream;

class AppliArgs 
{
    protected:
        QHash<QString, quint8>  _args;
        QHash<QChar, QString>   _shortArgs;
        QList<QString>          _optCanAlterParamsNr;

        QString                 _appliName;
        QHash<QString, QStringList> _values;
        QStringList             _aloneArgs;
        QStringList             _errors;

    public:
        AppliArgs();
        AppliArgs(const QStringList& args);

    public:
        QString                 applicationName() const {return _appliName;}
        bool                    isOptionSet(const QString& name) const {return 
                                    _values.contains(name) ? true : false; }
        QString                 optionArg(const QString& name, quint8 nr) const
                                    {return _values.value(name).at(nr);}
        QString                 parameter(quint8 nr) const {return _aloneArgs.
                                    at(nr); }
    public:
        void                    addSupportedArgs(const QString& longArg, 
                                    const QChar& shortArg=0, quint8 nrVal=0);
        void                    addSupportedArgs(const QStringList& args);
        bool                    parse(int argc, char** argv, quint32 maxParams);
        void                    printErrors(QTextStream& out) const;
};


#endif /* _APPLIARGS_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8 : */

