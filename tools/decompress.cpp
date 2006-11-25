/*
 *  decompress.cpp              (C) 2006, Aurélien Croc (AP¹C)
 *
 * Decompress a SPL2 or SPLc document into several pictures
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

/*
 * Définitions et déclarations
 * Definitions and declarations
 */
#define HEADER_SIGNATURE    0x00
#define HEADER_YRESOLUTION  0x01
#define HEADER_NRCOPIES_H   0x02
#define HEADER_NRCOPIES_L   0x03
#define HEADER_PAPERTYPE    0x04
#define HEADER_WIDTH_H      0x05
#define HEADER_WIDTH_L      0x06
#define HEADER_HEIGHT_H     0x07
#define HEADER_HEIGHT_L     0x08
#define HEADER_PAPERSOURCE  0x09
#define HEADER_DUPLEX       0x0B
#define HEADER_TUMBLE       0x0C
#define HEADER_XRESOLUTION  0x10

#define BAND_NUMBER         0x1
#define BAND_WIDTH_H        0x2
#define BAND_WIDTH_L        0x3
#define BAND_HEIGHT_H       0x4
#define BAND_HEIGHT_L       0x5

#define _(X)        X
typedef unsigned long   ptr_t;
typedef uint32_t flags_t;

typedef struct {
    bool            notFirstPage;
    uint8_t         page;
    FILE            *black;
    FILE            *cyan;
    FILE            *magenta;
    FILE            *yellow;
    int32_t         band;
    uint32_t        width;
    uint32_t        height;
    uint8_t         color;
} document;

enum Flags {
    Color           = 0x00000001,
};

enum Colors {
    Cyan            = 1,
    Magenta         = 2,
    Yellow          = 3,
    Black           = 4,
};

static char _empty[1024];




/*
 * Fonctions locales
 * Local functions
 */
bool __writePicture(FILE *input, int page, const char *color, uint16_t width,
    uint16_t height)
{
    char filename[32], buffer[8192];
    FILE *output;

    snprintf((char *)&filename, sizeof(filename), "page%04i-%s.pbm",
              page, color);
    if (!(output = fopen((const char *)&filename, "w"))) {
        fprintf(stderr, _("Cannot create file %s\n"), filename);
        return false;
    }
    fprintf(output, "P4\n");
    fprintf(output, "# Page %i ~ %s layer --- (C) decompress, 2006, Aurélien "
        "Croc (AP²C)\n", page, color);
    fprintf(output, "%i %i\n", width, height);
    fseek(input, 0, SEEK_SET);
    while (!feof(input)) {
        size_t toWrite;

        toWrite = fread((char *)&buffer, 1, sizeof(buffer), input);
        fwrite((char *)&buffer, 1, toWrite, output);
    }
    fclose(output);
    printf("Registering page to %s\n", filename);
    return true;
}

bool __addMissingBands(flags_t flags, document *doc, int32_t band,
    uint16_t height, uint16_t width)
{
    uint32_t size;

    doc->band++;
    if (doc->band == band)
        return true;
    size = (band - doc->band) * ((height * width + 7) >> 3);

    while (size >= sizeof(_empty)) {
        if (flags & Color) {
            fwrite((char *)&_empty, 1, sizeof(_empty), doc->cyan);
            fwrite((char *)&_empty, 1, sizeof(_empty), doc->magenta);
            fwrite((char *)&_empty, 1, sizeof(_empty), doc->yellow);
        }
        fwrite((char *)&_empty, sizeof(_empty), 1, doc->black);
        size -= sizeof(_empty);
    }
    if (size) {
        if (flags & Color) {
            fwrite((char *)&_empty, 1, size, doc->cyan);
            fwrite((char *)&_empty, 1, size, doc->magenta);
            fwrite((char *)&_empty, 1, size, doc->yellow);
        }
        fwrite((char *)&_empty, 1, size, doc->black);
    }
    doc->height += (band - doc->band) * height;
    doc->band = band;

    return true;
}

bool __sanitizeColorBand(FILE *output, uint16_t width, uint16_t height)
{
    uint32_t size;

    size = (width * height + 7) >> 3;
    while (size >= sizeof(_empty)) {
       fwrite((char *)&_empty, 1, sizeof(_empty), output);
       size -= sizeof(_empty);
    }
    if (size)
        fwrite((char *)&_empty, 1, size, output);
    return true;
}

bool __decompressBand(FILE *input, FILE *output, uint16_t width,
    uint16_t height)
{
    unsigned char buffer[0x80];
    unsigned char *out, *out2;
    unsigned long checksum=0, givenCS;
    uint32_t size, bandSize;
    short occurTable[0x40];
    uint32_t lastOccur;
    ptr_t i, j;

    // Prepare the output buffer
    bandSize = (width * height + 7) >> 3;
    out = new unsigned char[bandSize];
    memset(out, 0, bandSize);

    // Extract the band size
    fread((char *)&buffer, 4, 1, input);
    size = (buffer[0] << 24) + (buffer[1] << 16) + (buffer[2] << 8) + buffer[3];
    if (size < 0x88) {
        fprintf(stderr, _("Strange band size... (%i)\n"), size);
        return false;
    }

    // Endian definition
    fread((unsigned char *)&buffer, 4, 1, input);
    if (buffer[0] ==  0x09 && buffer[1] == 0xAB && buffer[2] == 0xCD &&
        buffer[3] == 0xEF) {
        fprintf(stderr, _("Big endian data not supported\n"));
        return false;
    }
    if (buffer[0] !=  0xEF || buffer[1] != 0xCD || buffer[2] != 0xAB ||
        buffer[3] != 0x09) {
        fprintf(stderr, _("Bad signature\n"));
        return false;
    }
    size -= 4;
    checksum = 0x09 + 0xAB + 0xCD + 0xEF;

    // Extract the header data and the occurrence table
    fread((unsigned char *)&buffer, 4, 1, input);
    lastOccur = buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) +
        (buffer[3] << 24);
    size -= 4;
    checksum += buffer[0] + buffer[1] + buffer[2] + buffer[3];

    fread((unsigned char *)&buffer, 2, 0x40, input);
    for (i=0; i < 0x40; i++) {
        occurTable[i] = ~(buffer[i*2] + (buffer[i*2 + 1] << 8));
        checksum += buffer[i*2] + buffer[i*2 + 1];
    }
    size -= 0x80;

    // Extract the first uncompressed data
    if (size < lastOccur) {
        fprintf(stderr, _("Invalid band size (%i)\n"), size);
        return false;
    }
    for (i=0; i < lastOccur; i++) {
        unsigned char c = fgetc(input);

        out[i] = c;
        checksum += c;
        size--;
    }

    // Decompress the other data
    size -= 4;
    while (size) {
        unsigned char counter = fgetc(input);

        checksum += counter;

        // Compressed
        if (counter & 0x80) {
            unsigned char number = fgetc(input);
            unsigned char *ref;
            size_t toRead;

            checksum += number;
            size -= 2;
            toRead = ((number & 0xC0) << 1) + (counter & 0x7F) + 3;
            number = number & ~0xC0;
            ref = out + i + 1;
            for (j=0; j < toRead; j++, i++)
                out[i] = ref[occurTable[number] + j];
        // Uncompressed
        } else {
            for (j = 0; j <= counter; j++, i++) {
                unsigned char c = fgetc(input);
                
                out[i] = c;
                checksum += c;
            }
            size -= counter + 2;
        }
    }

    // Check the checksum
    fread((char *)&buffer, 4, 1, input);
    givenCS = (buffer[0] << 24) + (buffer[1] << 16) + (buffer[2] << 8) +
        buffer[3];
    if (checksum != givenCS) {
        fprintf(stderr, _("Invalid checksum... data corrupted?\n"));
        delete[] out;
        return false;
    }

    // Rotate the data
    out2 = new unsigned char[bandSize];
    memset(out2, 0, bandSize);
    for (i = 0; i < bandSize; i++) {
        uint32_t x, y;

        x = i / height;
        y = i % height;
        out2[x + y * (width / 8)] = ~out[i];
    }

    // Write it in the file
    fwrite(out2, 1, bandSize, output);
    delete[] out;
    delete[] out2;

    return true;
}

bool _extractPage(FILE *input, flags_t flags, document *doc)
{
    unsigned char header[6];
    char signature;
    uint8_t band;

    // Create the temporary files
    doc->black = tmpfile();
    if (flags & Color) {
        doc->cyan = tmpfile();
        doc->magenta = tmpfile();
        doc->yellow = tmpfile();
    }

    doc->band = -1;
    printf(_("Decompressing"));
    while (1) {
        uint16_t width, height;

        if (feof(input)) {
            fprintf(stderr, _("End of file..F\n"));
            return false;
        }
        signature = fgetc(input);

        // End of page?
        if (signature == 0x1)
            break;
        // Not a new band?
        else if (signature != 0xC) {
            fprintf(stderr, _("Bad header signature (%i)\n"), signature);
            return false;
        }

        // Extract and decompress the band
        fread((char *)&header[1], 1, 5, input);
        band = header[BAND_NUMBER];
        width = (header[BAND_WIDTH_H] << 8) + header[BAND_WIDTH_L];
        height = (header[BAND_HEIGHT_H] << 8) + header[BAND_HEIGHT_L];
        __addMissingBands(flags, doc, band, height, width);
        if (!doc->width)
            doc->width = width;
        doc->height += height;
        printf(".", header[BAND_HEIGHT_H], header[BAND_HEIGHT_L]);

        if (flags & Color) {
            char color = fgetc(input);
            uint8_t colorMask = 0;
            char compVersion;

            while (color > 0 && color < 5) {
                FILE *output;

                if ((1 << color) & colorMask) {
                    fprintf(stderr, _("Color still specified for this band "
                        "(%i!\n"), color);
                    return false;
                }
                colorMask |= (1 << color);

                compVersion = fgetc(input);
                if (compVersion != 0x11) {
                    fprintf(stderr, _("Invalid compression version (%i)\n"),
                        compVersion);
                    return false;
                }
                if (color == Cyan)
                    output = doc->cyan;
                else if (color == Magenta)
                    output = doc->magenta;
                else if (color == Yellow)
                    output = doc->yellow;
                else
                    output = doc->black;
                if (__decompressBand(input, output, width, height) == false)
                    return false;
                color = fgetc(input);
            };
            if (color > 4) {
                fprintf(stderr, _("Invalid color layer (%i)\n"), color);
                return false;
            }

            // Sanitize the color band
            if (!((1 << Cyan) & colorMask))
                __sanitizeColorBand(doc->cyan, width, height);
            if (!((1 << Magenta) & colorMask))
                __sanitizeColorBand(doc->magenta, width, height);
            if (!((1 << Yellow) & colorMask))
                __sanitizeColorBand(doc->yellow, width, height);
            if (!((1 << Black) & colorMask))
                __sanitizeColorBand(doc->black, width, height);
        } else {
            char compVersion = fgetc(input);

            if (compVersion != 0x11) {
                fprintf(stderr, _("Invalid compression version (%i)\n"),
                        compVersion);
                return false;
            }
            if (__decompressBand(input, doc->black, width, height) == false)
                return false;
        }
    };
    printf("\n");

    // Create the different files
    if (flags & Color) {
        __writePicture(doc->cyan, doc->page, _("cyan"), doc->width,
            doc->height);
        fclose(doc->cyan);
        __writePicture(doc->magenta, doc->page, _("magenta"), doc->width,
            doc->height);
        fclose(doc->magenta);
        __writePicture(doc->yellow, doc->page, _("yellow"), doc->width,
            doc->height);
        fclose(doc->yellow);
    }
    __writePicture(doc->black, doc->page, _("black"), doc->width, doc->height);
    fclose(doc->black);
}

bool _extractHeader(FILE *input, flags_t flags, document *doc)
{
    uint16_t xres, yres, width, height, nrcopies;
    const char *papersource;
    const char *papertype;
    bool next = false;
    unsigned char header[0x11];

    // Extract the PJL header
    if (!doc->notFirstPage) {
        while (!feof(input)) {
            char c = fgetc(input);

            if (c == '\n' || c == '\r')
                next = true;
            else if (c == 0 && next == true)
                break;
        }
    }
    if (feof(input)) {
        fprintf(stderr, _("End of file..F\n"));
        return false;
    }

    // Read the header
    if (!doc->notFirstPage) {
        fread((char *)&header[1], 1, 0x10, input);
        doc->notFirstPage = true;
    } else
        fread((char *)&header[0], 1, 0x11, input);

    // Analyse the header
    // Resolution
    if (!header[HEADER_XRESOLUTION])
        xres = header[HEADER_YRESOLUTION]*100;
    yres = header[HEADER_YRESOLUTION]*100;
    // Paper type
    switch (header[HEADER_PAPERTYPE]) {
        case 0:
            papertype = _("Letter"); break;
        case 1:
            papertype = _("Legal"); break;
        case 2:
            papertype = _("A4"); break;
        case 3:
            papertype = _("Executive"); break;
        case 4:
            papertype = _("Ledger"); break;
        case 5:
            papertype = _("A3"); break;
        case 6:
            papertype = _("Com10"); break;
        case 7:
            papertype = _("Monarch"); break;
        case 8:
            papertype = _("C5"); break;
        case 9:
            papertype = _("DL"); break;
        case 10:
            papertype = _("JB4"); break;
        case 11:
            papertype = _("JB5"); break;
        case 12:
            papertype = _("B5"); break;
        case 13:
            papertype = _("Not listed"); break;
        case 14:
            papertype = _("JPost"); break;
        case 15:
            papertype = _("JDouble"); break;
        case 16:
            papertype = _("A5"); break;
        case 17:
            papertype = _("A6"); break;
        case 18:
            papertype = _("JB6"); break;
        case 21:
            papertype = _("Custom"); break;
        case 23:
            papertype = _("C6"); break;
        case 24:
            papertype = _("Folio"); break;
        default:
            papertype = _("unknown");
    };
    // Paper source
    switch (header[HEADER_PAPERSOURCE]) {
        case 1:
            papersource = _("Auto"); break;
        case 2:
            papersource = _("Manual"); break;
        case 3:
            papersource = _("Multi"); break;
        case 4:
            papersource = _("Top"); break;
        case 5:
            papersource = _("Lower"); break;
        case 6:
            papersource = _("Envelopes"); break;
        case 7:
            papersource = _("Third"); break;
        default:
            papersource = _("unknown");
    };
    // Printable area size
    width = (header[HEADER_WIDTH_H] << 8) + header[HEADER_WIDTH_L];
    height = (header[HEADER_HEIGHT_H] << 8) + header[HEADER_HEIGHT_L];
    // Copies number
    nrcopies = (header[HEADER_NRCOPIES_H] << 8) + header[HEADER_NRCOPIES_L];

    
    printf(_("New page: \n"));
    printf(_("     Number of copies: %i\n"), nrcopies);
    printf(_("     Resolution: %i×%i\n"), xres, yres);
    printf(_("     Paper type: %s\n"), papertype);
    printf(_("     Paper source: %s\n"), papersource);
    if (width || height)
        printf(_("     Printable area: %i×%i\n"), width, height);
    printf(_("     Duplex: %i\n"), header[HEADER_DUPLEX]);
    printf(_("     Duplex tumble: %i\n"), header[HEADER_TUMBLE]);

    return true;
}






/*
 * Programme principal
 * Main program
 */
int main(int argc, char **argv)
{
    const char *filename;
    flags_t flags=0;
    document *doc;
    FILE *handle;
    ptr_t i=1, j;
    
    // Get the command line options
    if (argc < 2) {
        fprintf(stderr, _("Usage: %s [<options>] <SPL file>\n"), argv[0]);
        fprintf(stderr, _("More information: %s -h\n"), argv[0]);
        return -1;
    }
    while (argv[i] && argv[i][0] == '-') {
        j=0;
        while (argv[i][j]) {
            switch (argv[i][j]) {
                case 'h':
                    printf(_("Usage: %s [<options>] <SPL file>\n"), argv[0]);
                    printf(_("Available options:\n"));
                    printf(_("      -2          This is a SPL2 document "
                        "[default]\n"));
                    printf(_("      -c          This is a SPLc document\n"));
                    printf(_("      -h          Print this help message\n"));
                    return 0;
                case '2':
                    flags &= ~Color;
                    break;
                case 'c':
                    flags |= Color;
                    break;
                default:
                    fprintf(stderr, _("Unknown option '%c'\n"), argv[i][j]);
                    break;
            };
            j++;
        }
        i++;
    }

    // Get the command line file name
    if (!argv[i]) {
        fprintf(stderr, _("Usage: %s [<options>] <SPL file>\n"), argv[0]);
        fprintf(stderr, _("More information: %s -h\n"), argv[0]);
        return 0;
    }
    filename = argv[i];

    // Open the file
    if (!(handle = fopen(filename, "r"))) {
        fprintf(stderr, _("Cannot open file %s (errno=%i)\n"), filename, errno);
        return errno;
    }

    // Extract the header
    doc = new document;
    memset(doc, 0, sizeof(document));
    if (_extractHeader(handle, flags, doc) == false)
        goto error;
    _extractPage(handle, flags, doc);

    delete doc;
    fclose(handle);
    return 0;

error:
    delete doc;
    fclose(handle);
    return -1;
}
