# SpliX Development Patterns

> CUPS printer driver for Samsung/Xerox/Dell QPDL printers

## Overview
SpliX is a Linux CUPS filter driver that converts raster data into the Samsung QPDL (Quick Page Description Language) byte-stream protocol. It supports ~100 printer models across Samsung, Xerox, Dell, Lexmark, and Toshiba brands.

## Architecture
- **Language**: C++23 (compiled with `-std=c++23`)
- **Build System**: CMake 3.16+ with presets (amd64, arm64, debug, ASan)
- **Testing**: Google Test (GTest) + functional shell tests + ASan/UBSan CI
- **Packaging**: CPack `.deb` for Debian/Ubuntu

## Key Binaries
| Binary | Purpose |
|--------|---------|
| `rastertoqpdl` | CUPS raster → QPDL converter (main filter) |
| `pstoqpdl` | PostScript preprocessor → chains to rastertoqpdl |

## Coding Conventions
- **Memory**: `std::unique_ptr`, `std::vector`, `std::span` — zero raw `new`/`delete`
- **Errors**: `SP::Result<T>` (alias for `std::expected<T, SP::Error>`)
- **Threads**: `std::jthread` with `stop_token`, `std::mutex`, `std::counting_semaphore`
- **Includes**: System headers via `<>`, project headers via `""` from `include/`
- **Comments**: Original French comments preserved alongside English translations

## Compression Algorithms
| ID | Class | Description |
|----|-------|-------------|
| 0x0D | `Algo0x0D` | Samsung SPL-C run-length (with 0x0E fallback) |
| 0x0E | `Algo0x0E` | Complementary to 0x0D |
| 0x11 | `Algo0x11` | LZS-variant compression |
| 0x13 | `Algo0x13` | JBIG whole-page compression |
| 0x15 | `Algo0x15` | JBIG banded compression (CLP-315, M2026) |

## Build Commands
```bash
# Native debug build
cmake --preset linux-debug && cmake --build --preset linux-debug

# Release + package
cmake --preset linux-amd64-release && cmake --build --preset linux-amd64-release
cd build-amd64 && cpack -G DEB

# Run tests
cd build-debug && ctest --output-on-failure

# ASan build
cmake --preset linux-asan && cmake --build --preset linux-asan
```

## Testing
- Test files: `tests/splix_gtest.cpp` (unit), `tests/functional_test.sh` (integration)
- CI runs: native amd64 + cross-compiled arm64 + ASan/UBSan sanitizer check
