bool QPDLDocument::_processBandAnalysis(quint8 color, quint16 width,
    quint16 height, quint8 compression, QByteArray& content,
    QTextStream& out, QTextStream& err)
{
    quint32 checkSum = 0, givenCheckSum;
    quint8 version;
    bool be;

    // Check the signature and the BE/LE
    if (((quint8)content.at(0) & 0xF) != 0x9 || (quint8)content.at(1) != 0xAB ||
        (quint8)content.at(2) != 0xCD || (quint8)content.at(3) != 0xEF) {
        if (((quint8)content.at(3) & 0xF) != 0x09 ||
            (quint8)content.at(2) != 0xAB || (quint8)content.at(1) != 0xCD ||
            (quint8)content.at(0) != 0xEF) {
            err << QString(_("QPDL: Invalid signature (0x%1%2%3%4)")).
                arg((quint8)content.at(0),0,16).arg((quint8)content.at(1),0,16).
                arg((quint8)content.at(2),0,16).arg((quint8)content.at(3),0,16)
                << endl;
            return false;
        } else {
            be = false;
            version = (quint8)content.at(3) >> 4;
        }
    } else {
        be = true;
        version = (quint8)content.at(0) >> 4;
    }
    _page.setSubHeaderVersion(version);
    _page.setBE(be);

    // Check the checksum
    if (_qpdl > 0) {
        givenCheckSum = ((quint8)content.at(content.size() - 4) << 24) +
            ((quint8)content.at(content.size() - 3) << 16) +
            ((quint8)content.at(content.size() - 2) << 8) +
            (quint8)content.at(content.size() - 1);
        for (unsigned int i=0; i < content.size() - 4; i++)
            checkSum += (quint8)content.at(i);
        if (givenCheckSum != checkSum) {
            err << QString(_("QPDL: band checksum invalid! (0x%1-0x%2)")).
                arg(givenCheckSum, 8, 16, QChar('0')).arg(checkSum, 8, 16,
                QChar('0')) << endl;
            return false;
        }
        content.remove(content.size() - 4, 4);
    }
    content.remove(0, 4);

    // Decompress the band
    if (_decompression || _dump)
        return _page.process(color, width, height, _dump ? 0 : compression,
            content, out, err);
    return true;
}



/*
 * Boucle générale de traitement des données
 * Data management main loop
 */
bool QPDLDocument::parse(QFile& data, QTextStream& out, QTextStream& err)
{
    _page.setQPDLDocument(this);

    while (!data.atEnd()) {
        QByteArray footer;
        quint16 nrCopies;
        Result res;

        _pageNr++;
        _page.clear();

        // Read the page header
        res = _readPageHeader(data, out, err);
        if (res == Error)
            return false;
        if (res == End)
            break;

        // Read the page bands
        res = _readPageContent(data, out, err);
        if (res == Error)
            return false;
        if (res == End)
            break;

        // Read the page footer
        footer = data.read(2);
        nrCopies = ((quint8)footer.at(0) << 8) + (quint8)footer.at(1);
        if (!_quiet)
            out << QString(_("Page %1 done (%2 copie(s)).")).arg(_pageNr).
                arg(nrCopies) << endl << endl;
        _page.flush();
    }
    return true;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8 : */
