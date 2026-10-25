#include "band.h"
#include "bandplane.h"

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

namespace {

int createTemporaryFile()
{
    char path[] = "/tmp/splix-serialization-XXXXXX";
    const int fd = mkstemp(path);
    if (fd >= 0)
        unlink(path);
    return fd;
}

BandPlane* createPlane()
{
    unsigned char* data = new unsigned char[4];
    data[0] = 0x11;
    data[1] = 0x22;
    data[2] = 0x33;
    data[3] = 0x44;
    BandPlane* plane = new BandPlane();
    plane->setColorNr(1);
    plane->setCompression(0x11);
    plane->setEndian(BandPlane::BigEndian);
    plane->setData(data, 4);
    return plane;
}

bool roundTripAndRejectTruncation()
{
    const int fd = createTemporaryFile();
    if (fd < 0)
        return false;
    Band original(3, 4960, 128);
    original.registerPlane(createPlane());
    if (!original.swapToDisk(fd) || lseek(fd, 0, SEEK_SET) < 0) {
        close(fd);
        return false;
    }
    Band* restored = Band::restoreIntoMemory(fd);
    const BandPlane* plane = restored ? restored->plane(0) : NULL;
    const unsigned char expected[] = {0x11, 0x22, 0x33, 0x44};
    const bool matches = restored && restored->bandNr() == 3 &&
        restored->width() == 4960 && restored->height() == 128 && plane &&
        plane->dataSize() == sizeof(expected) &&
        std::memcmp(plane->data(), expected, sizeof(expected)) == 0;
    delete restored;

    const off_t length = lseek(fd, 0, SEEK_END);
    const bool truncated = length > 0 && ftruncate(fd, length - 1) == 0 &&
        lseek(fd, 0, SEEK_SET) >= 0;
    Band* invalid = truncated ? Band::restoreIntoMemory(fd) : NULL;
    const bool rejected = truncated && invalid == NULL;
    delete invalid;
    close(fd);
    return matches && rejected;
}

} // namespace

int main()
{
    if (!roundTripAndRejectTruncation()) {
        std::cerr << "serialization regression failed\n";
        return 1;
    }
    std::cout << "serialization tests passed\n";
    return 0;
}
