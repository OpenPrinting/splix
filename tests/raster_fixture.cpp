#include <cups/raster.h>

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

constexpr unsigned kResolution = 600;
constexpr unsigned kPageWidthPoints = 420;
constexpr unsigned kPageHeightPoints = 595;
constexpr unsigned kRasterWidth = 3500;
constexpr unsigned kRasterHeight = 4958;

bool writePage(cups_raster_t* raster)
{
    cups_page_header2_t header{};
    header.HWResolution[0] = kResolution;
    header.HWResolution[1] = kResolution;
    header.PageSize[0] = kPageWidthPoints;
    header.PageSize[1] = kPageHeightPoints;
    header.NumCopies = 1;
    header.cupsWidth = kRasterWidth;
    header.cupsHeight = kRasterHeight;
    header.cupsBitsPerColor = 1;
    header.cupsBitsPerPixel = 1;
    header.cupsBytesPerLine = (kRasterWidth + 7) / 8;
    header.cupsColorOrder = CUPS_ORDER_CHUNKED;
    header.cupsColorSpace = CUPS_CSPACE_K;
    header.cupsCompression = 0x11;

    if (!cupsRasterWriteHeader2(raster, &header))
        return false;

    std::vector<unsigned char> row(header.cupsBytesPerLine, 0);
    for (unsigned y = 0; y < kRasterHeight; ++y) {
        std::fill(row.begin(), row.end(), 0);
        if (y % 257 == 0)
            row[(y / 257) % row.size()] =
                static_cast<unsigned char>(0x80U >> (y % 8));
        if (cupsRasterWritePixels(raster, row.data(), row.size()) != row.size())
            return false;
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "usage: raster_fixture OUTPUT\n";
        return 2;
    }

    const int fd = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY, 0600);
    if (fd < 0)
        return 3;

    cups_raster_t* raster = cupsRasterOpen(fd, CUPS_RASTER_WRITE);
    const bool ok = raster && writePage(raster);
    if (raster)
        cupsRasterClose(raster);
    close(fd);
    return ok ? 0 : 4;
}
