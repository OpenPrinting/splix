/* Checked POSIX I/O helpers. Copyright (C) 2026 Michael Maertzdorf. */
#ifndef SPLIX_IO_UTILS_H
#define SPLIX_IO_UTILS_H

#include <cstddef>

namespace splix {

bool readAll(int fd, void* buffer, std::size_t size);
bool writeAll(int fd, const void* buffer, std::size_t size);
bool canRead(int fd, std::size_t size);

} // namespace splix

#endif /* SPLIX_IO_UTILS_H */
