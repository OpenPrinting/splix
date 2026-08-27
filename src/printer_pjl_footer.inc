bool Printer::sendPJLFooter([[maybe_unused]] const Request& request) const
{
    printf("%s", _endPJL.c_str());
    fflush(stdout);

    return true;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */
