QPDLDocument::Result QPDLDocument::_readPageContent(QFile& data,
    QTextStream& out, QTextStream& err)
{
    if (!_quiet) {
        if (_decompression)
            out << _("    Analysing and decompression");
        else
            out << _("    Analysing");
    }

    while (!data.atEnd()) {
        QByteArray header, content;
        quint16 width, height;
        char signature, color=1;
        quint8 compression;
        quint32 size;

        // Check the band signature
        data.getChar(&signature);
        if (signature == 0x1) { // End of the page
            if (!_quiet)
                out << QString("(0x%1)").arg(compression, 0, 16) << endl;
            return Ok;
        } else if (signature != 0xC) {
            err << QString(_("QPDL: bad band header signature (%1)")).
                arg((quint8)signature) << endl;
            return Error;
        }

        // Extract the band information
        header = data.read(5);
        _currentBandNr = header.at(BAND_NUMBER);
        width = ((quint8)header.at(BAND_WIDTH_H) << 8) +
            (quint8)header.at(BAND_WIDTH_L);
        height = ((quint8)header.at(BAND_HEIGHT_H) << 8) +
            (quint8)header.at(BAND_HEIGHT_L);
        if (!_quiet) {
            out << '.';
            out.flush();
        }

        // Read the band data -- SPLc in QPDL V. 0, 1 or 5
        if (_type == SPLc && (_qpdl <= 1 || _qpdl == 5)) {
            while (!data.atEnd()) {
                // Read the color
                data.getChar(&color);
                if (!color)
                    break;
                if (color < 0 || color > 4) {
                    err << QString(_("QPDL: bad color value (%1) in band %2")).
                        arg((quint8)color).arg(_currentBandNr) << endl;
                    return Error;
                }

                // Read the compression and the size
                header = data.read(5);
                compression = header.at(0);
                size = ((quint8)header.at(1) << 24) +
                    ((quint8)header.at(2) << 16) +
                    ((quint8)header.at(3) << 8) + (quint8)header.at(4);
                content = data.read(size);

                // Process the band
                if (!_processBandAnalysis(color, width, height, compression,
                    content, out, err))
                    return Error;
            }


        // Read the band data -- SPLc in QPDL V. 2
        } else if (_type == SPLc && _qpdl == 2) {
            // Read the color
            data.getChar(&color);
            if (color <= 0 || color > 4) {
                err << QString(_("QPDL: bad color value (%1) in band %2")).
                    arg((quint8)color).arg(_currentBandNr) << endl;
                return Error;
            }

            // Read the compression and the size
            header = data.read(5);
            compression = header.at(0);
            size = ((quint8)header.at(1) << 24) + ((quint8)header.at(2) << 16) +
                ((quint8)header.at(3) << 8) + (quint8)header.at(4);
            content = data.read(size);

            // Process the band
            if (!_processBandAnalysis(color, width, height, compression,
                        content, out, err))
                return Error;


        // Read the band data -- SPL-2 in any QPDL
        } else {
            // Read the compression and the size
            header = data.read(5);
            compression = header.at(0);
            size = ((quint8)header.at(1) << 24) + ((quint8)header.at(2) << 16) +
                ((quint8)header.at(3) << 8) + (quint8)header.at(4);
            content = data.read(size);

            // Process the band
            if (!_processBandAnalysis(1, width, height, compression,
                        content, out, err))
                return Error;
        }
    }
    return Error;
}



/*
 * Traitement d'une bande
 * Band management
 */
