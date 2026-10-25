/* Checked POSIX I/O helpers. Copyright (C) 2026 Michael Maertzdorf. */
#include "io_utils.h"

#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>

namespace splix {

bool readAll(int fd, void* buffer, std::size_t size)
{
    unsigned char* next = static_cast<unsigned char*>(buffer);
    while (size != 0) {
        const ssize_t count = read(fd, next, size);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0)
            return false;
        next += count;
        size -= static_cast<std::size_t>(count);
    }
    return true;
}

bool writeAll(int fd, const void* buffer, std::size_t size)
{
    const unsigned char* next = static_cast<const unsigned char*>(buffer);
    while (size != 0) {
        const ssize_t count = write(fd, next, size);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0)
            return false;
        next += count;
        size -= static_cast<std::size_t>(count);
    }
    return true;
}

bool canRead(int fd, std::size_t size)
{
    struct stat info;
    const off_t position = lseek(fd, 0, SEEK_CUR);
    if (position < 0 || fstat(fd, &info) != 0 || info.st_size < position)
        return false;
    return size <= static_cast<std::size_t>(info.st_size - position);
}

} // namespace splix
