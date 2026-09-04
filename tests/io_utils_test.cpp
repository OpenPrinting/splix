#include "io_utils.h"

#include <array>
#include <iostream>
#include <unistd.h>

int main()
{
    int descriptors[2];
    if (pipe(descriptors) != 0)
        return 1;

    const std::array<unsigned char, 4> source = {0x11, 0x22, 0x33, 0x44};
    std::array<unsigned char, 4> destination = {};
    const bool wrote = splix::writeAll(descriptors[1], source.data(),
                                       source.size());
    close(descriptors[1]);
    const bool read = splix::readAll(descriptors[0], destination.data(),
                                     destination.size());
    const bool eofFails = !splix::readAll(descriptors[0], destination.data(), 1);
    close(descriptors[0]);

    if (!wrote || !read || !eofFails || destination != source) {
        std::cerr << "checked I/O regression failed\n";
        return 1;
    }
    std::cout << "checked I/O tests passed\n";
    return 0;
}
