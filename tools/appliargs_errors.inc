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
