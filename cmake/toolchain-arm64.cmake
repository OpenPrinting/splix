# cmake/toolchain-arm64.cmake
#
# CMake toolchain file for cross-compiling SpliX to ARM64 (aarch64) on a
# Debian/Ubuntu x86-64 (amd64) host.
#
# Usage — local build:
#   cmake --preset linux-arm64-release
#
# Or manually:
#   cmake -S . -B build-arm64 \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm64.cmake \
#         -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-arm64 --parallel
#
# Prerequisites (install once on the build host):
#   dpkg --add-architecture arm64
#   apt-get update
#   apt-get install \
#       gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
#       cups libcups2-dev \
#       libcups2-dev:arm64 libcupsimage2-dev:arm64 libjbig-dev:arm64
#
# How it works:
#   CMake's toolchain file is processed before CMakeLists.txt.  It tells CMake
#   which compiler to use, where the target system's libraries live (via
#   CMAKE_FIND_ROOT_PATH), and how to configure pkg-config so that the CUPS
#   path queries return arm64 paths rather than x86-64 paths.

# ── Target System ────────────────────────────────────────────────────────────
set(CMAKE_SYSTEM_NAME      Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# ── Cross Compiler ───────────────────────────────────────────────────────────
# Provided by: apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# ── Optional: QEMU emulator for CTest ────────────────────────────────────────
# If qemu-aarch64-static is installed, CMake can use it to run ARM64
# binaries during `ctest`, enabling smoke tests even when cross-compiling.
# This is entirely optional — tests are skipped if the emulator is absent.
find_program(_QEMU_ARM64 NAMES qemu-aarch64-static qemu-aarch64)
if(_QEMU_ARM64)
    set(CMAKE_CROSSCOMPILING_EMULATOR "${_QEMU_ARM64}")
endif()

# ── Library / Header Search Paths ────────────────────────────────────────────
# On Debian/Ubuntu, multiarch arm64 packages install to:
#   libraries : /usr/lib/aarch64-linux-gnu/
#   headers   : /usr/include/aarch64-linux-gnu/
#
# We use CMAKE_FIND_ROOT_PATH to prioritize the arm64 multiarch directories
# so CMake finds the aarch64 libraries before any host (amd64) libraries.
set(CMAKE_FIND_ROOT_PATH
    /usr/lib/aarch64-linux-gnu
    /usr/include/aarch64-linux-gnu
)

# Search mode rationale:
#   PROGRAM : NEVER — use host tools (cmake, make, pkg-config) only
#   LIBRARY : BOTH  — prioritize arm64 multiarch dirs but allow fallback to
#                      /usr/lib for architecture-independent libraries
#   INCLUDE : BOTH  — prioritize arm64 headers but allow /usr/include where
#                      multiarch packages put shared (arch-independent) headers
#   PACKAGE : ONLY  — ensure find_package doesn't pick up host x86-64 configs
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ── pkg-config Cross-Compilation Override ────────────────────────────────────
# By default pkg-config looks at the host's .pc files (/usr/lib/x86_64-linux-gnu).
# We redirect it to the arm64 .pc files so that:
#   - pkg_check_modules(CUPS REQUIRED cups) finds the arm64 CUPS library
#   - execute_process(pkg-config --variable=cups_serverbin cups) returns the
#     correct arm64-aware install path
#
# PKG_CONFIG_LIBDIR  : replaces (not extends) the default search path
# PKG_CONFIG_PATH    : cleared so host paths don't leak in
# PKG_CONFIG_SYSROOT_DIR: not set — Debian multiarch doesn't use a sysroot
set(ENV{PKG_CONFIG_PATH}    "")
set(ENV{PKG_CONFIG_LIBDIR}  "/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig")
