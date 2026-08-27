static SP::Result<> _renderBand(const Request& request, const Band* band, bool mono)
{
    unsigned long version, subVersion, size, dataSize, checkSum;
    bool color, headerSent=false;
    std::array<unsigned char, 0x20> header;
    version = request.printer()->qpdlVersion();
    color = request.printer()->color();
    subVersion = band->parent()->compression() == 0x13 ? 3 : 0;

    size_t planeIdx = 0;
    for (const BandPlane* plane : band->planes()) {
        unsigned long compression;
        bool nextBand = false;

        if (!plane) {
            ERRORMSG(_("Inconsistent data. Operation aborted"));
            return SP::Unexpected(SP::Error::InconsistentData);
        }
        compression = plane->compression();
        checkSum = plane->checksum();

        // Check if there is a next band for that color
        if (subVersion) {
            const BandPlane *nextPlane;
            const Band *next;

            next = band->sibling();
            if (next) {
                for (unsigned int j=0; j < next->planesNr(); j++) {
                    nextPlane = next->plane(j);
                    if (nextPlane && nextPlane->colorNr() == plane->colorNr()){
                        nextBand = true;
                        break;
                    }
                }
            }
        }

        // Calculate the data size
        dataSize = static_cast<unsigned long>(plane->data_span().size());
        if (compression != 0x0D && compression != 0x0E)
            dataSize += 4;              // Data signature
        if (version > 0) {
            dataSize += 4;              // Checksum
            if (subVersion == 3)
                dataSize += 7*4;        // Sub-header
        }

        // Send the header
        if (!headerSent || version == 2) {
            header[0x0] = 0xC;                      // Signature
            header[0x1] = static_cast<unsigned char>(band->bandNr());           // Band number
            header[0x2] = static_cast<unsigned char>(band->width() >> 8);       // Band width 8-15
            header[0x3] = static_cast<unsigned char>(band->width());            // Band width 0-7
            header[0x4] = static_cast<unsigned char>(band->height() >> 8);      // Band height 8-15
            header[0x5] = static_cast<unsigned char>(band->height());           // Band height 0-7
            headerSent = true;
            size = 0x6;
        } else
            size = 0x0;
        // Add color information if it's a color printer
        if (color) {
            header[size] = static_cast<unsigned char>(mono ? 4 : plane->colorNr()); // Color number
            size++;
        }
        // Append the last information and send the header
        header[size+0] = static_cast<unsigned char>(compression);               // Compression algorithm
        header[size+1] = static_cast<unsigned char>(dataSize >> 24);            // Data size 24 - 31
        header[size+2] = static_cast<unsigned char>(dataSize >> 16);            // Data size 16 - 23
        header[size+3] = static_cast<unsigned char>(dataSize >> 8);             // Data size 8 - 15
        header[size+4] = static_cast<unsigned char>(dataSize);                  // Data size 0 - 7
        if (auto res = _safeWrite(std::span(header.data(), size+5)); !res) return res;

        // Send the sub-header
        if (compression != 0x0D && compression != 0x0E) {
            switch (plane->endian()) {
                case BandPlane::Endian::Dependant: {
                    // Machine-native byte order — must match original uint32_t* cast behavior
                    uint32_t sig = static_cast<uint32_t>(0x09ABCDEF + (subVersion << 28));
                    memcpy(&header[0x0], &sig, sizeof(sig));
                    break;
                }
                case BandPlane::Endian::BigEndian:
                    header[0x0] = static_cast<unsigned char>(0x9 + (subVersion << 0x4));
                                                        // Sub-header signature1
                    header[0x1] = 0xAB;                 // Sub-header signature2
                    header[0x2] = 0xCD;                 // Sub-header signature3
                    header[0x3] = 0xEF;                 // Sub-header signature4
                    break;
                case BandPlane::Endian::LittleEndian:
                    header[0x0] = 0xEF;                 // Sub-header signature4
                    header[0x1] = 0xCD;                 // Sub-header signature3
                    header[0x2] = 0xAB;                 // Sub-header signature2
                    header[0x3] = static_cast<unsigned char>(0x9 + (subVersion << 0x4));
                                                        // Sub-header signature1
                    break;
            };
            size = 4;
            if (subVersion == 3) {
                uint32_t state;
                auto pSpan = plane->data_span();
                size_t dSize = pSpan.size();

                checkSum += 0x39 + 0xAB + 0xCD + 0xEF;
                if (!band->bandNr())
                    state = 0x0;                        // First band
                else if (nextBand) {
                    state = 0x01000000;                 // Next band available
                    checkSum += 0x01;
                } else {
                    state = 0x02000000;                 // Last band
                    checkSum += 0x02;
                }
                memset(header.data() + size + 4, 0, 6 * 4);

                switch (plane->endian()) {
                    case BandPlane::Endian::Dependant: {
                        // Machine-native byte order — must match original uint32_t* cast behavior
                        uint32_t udSize = static_cast<uint32_t>(dSize);
                        memcpy(&header[size], &udSize, sizeof(udSize));
                        memcpy(&header[size + 4], &state, sizeof(state));
                        break;
                    }
                    case BandPlane::Endian::BigEndian:
                        // Data size 24 - 31
                        header[size+0] = static_cast<unsigned char>(dSize >> 24);
                        // Data size 16 - 23
                        header[size+1] = static_cast<unsigned char>(dSize >> 16);
                        // Data size 8 - 15
                        header[size+2] = static_cast<unsigned char>(dSize >> 8);
                        // Data size 0 - 7
                        header[size+3] = static_cast<unsigned char>(dSize);
                        // State 24 - 31
                        header[size+4] = static_cast<unsigned char>(state >> 24);
                        // State 16 - 23
                        header[size+5] = static_cast<unsigned char>(state >> 16);
                        // State 8 - 15
                        header[size+6] = static_cast<unsigned char>(state >> 8);
                        // State 0 - 7
                        header[size+7] = static_cast<unsigned char>(state);
                        break;
                    case BandPlane::Endian::LittleEndian:
                        // Data size 0 - 7
                        header[size+0] = static_cast<unsigned char>(dSize);
                        // Data size 8 - 15
                        header[size+1] = static_cast<unsigned char>(dSize >> 8);
                        // Data size 16 - 23
                        header[size+2] = static_cast<unsigned char>(dSize >> 16);
                        // Data size 24 - 31
                        header[size+3] = static_cast<unsigned char>(dSize >> 24);
                        // State 0 - 7
                        header[size+4] = static_cast<unsigned char>(state);
                        // State 8 - 15
                        header[size+5] = static_cast<unsigned char>(state >> 8);
                        // State 16 - 23
                        header[size+6] = static_cast<unsigned char>(state >> 16);
                        // State 24 - 31
                        header[size+7] = static_cast<unsigned char>(state >> 24);
                        break;
                }
                for (unsigned int j=0; j < 4; j++)
                    checkSum += header[size + j];
                size += 4 + 4 + 5*4;
            } else
                for (unsigned int j=0; j < 4; j++)
                    checkSum += header[size - j - 1];
            if (auto res = _safeWrite(std::span(header.data(), size)); !res) return res;
        }

        // Send the data
        if (auto res = _safeWrite(plane->data_span()); !res) return res;

        // Send the checksum
        header[0] = static_cast<unsigned char>(checkSum >> 24);                 // Checksum 24 - 31
        header[1] = static_cast<unsigned char>(checkSum >> 16);                 // Checksum 16 - 23
        header[2] = static_cast<unsigned char>(checkSum >> 8);                  // Checksum 8 - 15
        header[3] = static_cast<unsigned char>(checkSum);                       // Checksum 0 - 7
        size = 4;
            // Close the plane if needed
        if (color && (version == 1 || version == 5) &&
            (planeIdx + 1) == band->planesNr()) {
            header[4] = 0;
            size++;
        }
        if (auto res = _safeWrite(std::span(header.data(), size)); !res) return res;
        planeIdx++;
    }

    return {};
}
