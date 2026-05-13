cmake_minimum_required(VERSION 3.16)

project(splix
    VERSION     2.0.2
    DESCRIPTION "Samsung / Xerox / Dell printer CUPS driver (QPDL)"
    LANGUAGES   CXX
)

# ────────────────────────────────────────────────────────────────────────────
# C++ Standard
# ────────────────────────────────────────────────────────────────────────────
# We target C++23 to leverage modern memory safety features like std::span,
# std::expected, and enhanced RAII.
set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ────────────────────────────────────────────────────────────────────────────
# Global Configuration Options & Hardening
# ────────────────────────────────────────────────────────────────────────────
include(cmake/SpliXHardening.cmake)
option(DISABLE_JBIG "Disable JBIG compression" OFF)
option(DISABLE_THREADS "Disable POSIX threads" OFF)
option(DISABLE_BLACKOPTIM "Disable black optimization" OFF)
set(THREADS "2" CACHE STRING "Number of threads to use")
set(CACHESIZE "30" CACHE STRING "Cache size in MB")

# ────────────────────────────────────────────────────────────────────────────
# Static Analysis & Tooling
# ────────────────────────────────────────────────────────────────────────────
find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE};-checks=-*,readability-*,bugprone-*,modernize-*,cppcoreguidelines-,-cppcoreguidelines-pro-type-member-init,-cppcoreguidelines-pro-type-static-cast-downcast)
    message(STATUS "Clang-Tidy enabled.")
endif()

find_program(CPPCHECK_EXE NAMES cppcheck)
if(CPPCHECK_EXE)
    set(CMAKE_CXX_CPPCHECK ${CPPCHECK_EXE};--enable=warning,performance,portability,style;--inconclusive;--force)
    message(STATUS "Cppcheck enabled.")
endif()

# ────────────────────────────────────────────────────────────────────────────
# Version header generation
# ────────────────────────────────────────────────────────────────────────────
# Generate version.h from the template so that the C++ code always matches
# the CMake project version.  The generated file goes into the build tree;
# the original include/version.h is kept untouched for users who build with
# the legacy Makefile.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/version.h"
    @ONLY
)

# ────────────────────────────────────────────────────────────────────────────
# Core Dependencies
# ────────────────────────────────────────────────────────────────────────────
# We use CMake's built-in `FindCups` module because it natively searches 
# the system paths for cups/cups.h and the libcups library. We specifically
# avoid `pkg_check_modules` because some older Debian configurations lack
# the cups.pc file but still provide the library and headers.

find_package(PkgConfig REQUIRED)

# FindCups provides CUPS_INCLUDE_DIRS and CUPS_LIBRARIES
find_package(Cups)
if(NOT CUPS_FOUND)
    message(FATAL_ERROR
        "\n[splix] CUPS development headers not found."
        "\nInstall them with:"
        "\n  apt-get install cups libcups2-dev\n"
    )
endif()

# libcupsimage — companion library shipped alongside CUPS but NOT listed in
# cups.pc; must be found separately.
# Debian/Ubuntu package: libcupsimage2-dev
find_library(CUPSIMAGE_LIBRARY
    NAMES   cupsimage
    HINTS   ${CUPS_LIBRARY_DIRS}
    DOC     "CUPS image conversion library (libcupsimage)"
)
if(NOT CUPSIMAGE_LIBRARY)
    message(FATAL_ERROR
        "\n[splix] libcupsimage not found."
        "\nInstall it with:"
        "\n  apt-get install libcupsimage2-dev\n"
    )
endif()

# libjbig — JBIG banded-compression used by algorithm 0x15 in rastertoqpdl.
# Modern Debian/Ubuntu ship it as -ljbig (package: libjbig-dev).
# The old module.mk called it -ljbig85 (Knoppix/ancient name) — that was
# patched to -ljbig as part of the 2025 build-system modernisation.
# Debian/Ubuntu package: libjbig-dev
find_library(JBIG_LIBRARY
    NAMES   jbig
    DOC     "JBIG compression library (libjbig)"
)
if(NOT JBIG_LIBRARY)
    message(FATAL_ERROR
        "\n[splix] libjbig not found."
        "\nInstall it with:"
        "\n  apt-get install libjbig-dev\n"
    )
endif()

# POSIX threads (pthreads or equivalent on the target platform)
find_package(Threads REQUIRED)

# ────────────────────────────────────────────────────────────────────────────
# CUPS Installation Path Variables
# ────────────────────────────────────────────────────────────────────────────
# Query the CUPS installation once at configure time so that:
#   a) The paths are baked into the compiled binaries (pstoqpdl uses execl()
#      with hardcoded strings).
#   b) The install() rules use the same paths as the CUPS subsystem expects.
#
# All six are exposed as CACHE STRING so a package maintainer can override
# them from the cmake command line without editing this file:
#   cmake -DSPLIX_FILTER_DIR=/usr/lib/cups/filter ..

find_program(CUPS_CONFIG_EXECUTABLE NAMES cups-config)
if(CUPS_CONFIG_EXECUTABLE)
    execute_process(
        COMMAND ${CUPS_CONFIG_EXECUTABLE} --serverbin
        OUTPUT_VARIABLE _CUPS_SERVERBIN
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND ${CUPS_CONFIG_EXECUTABLE} --datadir
        OUTPUT_VARIABLE _CUPS_DATADIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    execute_process(
        COMMAND pkg-config --variable=cups_serverbin cups
        OUTPUT_VARIABLE _CUPS_SERVERBIN
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND pkg-config --variable=cups_datadir cups
        OUTPUT_VARIABLE _CUPS_DATADIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

if(NOT _CUPS_SERVERBIN)
    set(_CUPS_SERVERBIN "/usr/lib/cups")
endif()
if(NOT _CUPS_DATADIR)
    set(_CUPS_DATADIR "/usr/share/cups")
endif()

set(SPLIX_FILTER_DIR  "${_CUPS_SERVERBIN}/filter"
    CACHE STRING "Directory where CUPS looks for filter binaries")
set(SPLIX_PPD_DIR     "${_CUPS_DATADIR}/model"
    CACHE STRING "Directory where CUPS looks for PPD driver files")
set(SPLIX_DRV_DIR     "${_CUPS_DATADIR}/drv"
    CACHE STRING "Directory where CUPS looks for DRV source files")
set(SPLIX_PROFILE_DIR "${_CUPS_DATADIR}/profiles"
    CACHE STRING "Directory where CUPS looks for colour profiles")

# Runtime names of the raster converters called by pstoqpdl via fork/exec.
# pstoqpdl tries gstoraster first (Ghostscript-native), falls back to pstoraster.
set(PSTORASTER_BIN "pstoraster"
    CACHE STRING "Name of the PostScript-to-raster CUPS filter")
set(GSTORASTER_BIN "gstoraster"
    CACHE STRING "Name of the Ghostscript-to-raster CUPS filter")

message(STATUS "SpliX install paths (override with -DSPLIX_*):")
message(STATUS "  Filter dir  : ${SPLIX_FILTER_DIR}")
message(STATUS "  PPD dir     : ${SPLIX_PPD_DIR}")
message(STATUS "  DRV dir     : ${SPLIX_DRV_DIR}")
message(STATUS "  Profile dir : ${SPLIX_PROFILE_DIR}")

# ────────────────────────────────────────────────────────────────────────────
# Core Static Library  (splix_core)
# ────────────────────────────────────────────────────────────────────────────
# Every .cpp in src/ that is NOT an entry-point binary; this becomes the
# shared core that both filter executables link against.
#
# Explicit file list (no GLOB) — CMake docs recommend this so that adding
# or removing a source file always triggers a reconfigure.
set(SPLIX_CORE_SOURCES
    src/algo0x0d.cpp
    src/algo0x0e.cpp
    src/algo0x11.cpp
    src/algo0x13.cpp
    src/algo0x15.cpp
    src/band.cpp
    src/bandplane.cpp
    src/cache.cpp
    src/colors.cpp
    src/compress.cpp
    src/core.cpp
    src/document.cpp
    src/page.cpp
    src/ppdfile.cpp
    src/printer.cpp
    src/qpdl.cpp
    src/rendering.cpp
    src/request.cpp
    src/sp_semaphore.cpp
)

add_library(splix_core STATIC ${SPLIX_CORE_SOURCES})
splix_apply_hardening(splix_core)
target_link_libraries(splix_core PUBLIC Cups::Cups Threads::Threads)

# Generated version.h (build dir) takes priority over original (source dir).
target_include_directories(splix_core PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${CUPS_INCLUDE_DIR}
    ${CUPS_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Propagate CUPS compile flags (e.g. -D_CUPS_SOURCE) to consumers
target_compile_options(splix_core PUBLIC ${CUPS_CFLAGS_OTHER})

# Thread/cache/JBIG/blackoptim definitions — scoped to splix_core (and its
# dependents via PUBLIC) instead of the old global add_compile_definitions().
if(DISABLE_THREADS)
    target_compile_definitions(splix_core PUBLIC DISABLE_THREADS)
else()
    target_compile_definitions(splix_core PUBLIC
        THREADS=${THREADS}
        CACHESIZE=${CACHESIZE}
    )
endif()

if(DISABLE_JBIG)
    target_compile_definitions(splix_core PUBLIC DISABLE_JBIG)
endif()

if(DISABLE_BLACKOPTIM)
    target_compile_definitions(splix_core PUBLIC DISABLE_BLACKOPTIM)
endif()

# ────────────────────────────────────────────────────────────────────────────
# Binary: rastertoqpdl
# ────────────────────────────────────────────────────────────────────────────
# Reads raster data from CUPS and writes a QPDL byte-stream to the printer.
# Links JBIG for the 0x13/0x15 compression algorithms.
add_executable(rastertoqpdl src/rastertoqpdl.cpp)
splix_apply_hardening(rastertoqpdl)
target_link_libraries(rastertoqpdl PRIVATE
    splix_core
    ${CUPS_LIBRARIES}       # -lcups
    ${CUPSIMAGE_LIBRARY}    # cupsImageOpen / cupsImageGetPixels
    ${JBIG_LIBRARY}         # jbg_enc_init / jbg_enc_out
    Threads::Threads
)
target_link_directories(rastertoqpdl PRIVATE ${CUPS_LIBRARY_DIRS})

# ────────────────────────────────────────────────────────────────────────────
# Binary: pstoqpdl
# ────────────────────────────────────────────────────────────────────────────
# PostScript pre-processor: inserts CMS colour correction data then chains to
# gstoraster/pstoraster → rastertoqpdl via fork/exec.
#
# CRITICAL: six paths must be baked in as compile-time string literals.
# Without these defines the file fails to compile:
#   pstoqpdl.cpp:69: error: 'RASTERDIR' undeclared
# They replace the src_pstoqpdl_cpp_FLAGS variable in the old module.mk.
add_executable(pstoqpdl src/pstoqpdl.cpp)
splix_apply_hardening(pstoqpdl)
target_compile_definitions(pstoqpdl PRIVATE
    RASTERDIR="${SPLIX_FILTER_DIR}"
    RASTERTOQPDL="rastertoqpdl"
    GSTORASTER="${GSTORASTER_BIN}"
    PSTORASTER="${PSTORASTER_BIN}"
    CUPSPPD="${SPLIX_PPD_DIR}"
    CUPSPROFILE="${SPLIX_PROFILE_DIR}"
)
target_link_libraries(pstoqpdl PRIVATE
    splix_core
    ${CUPS_LIBRARIES}
    ${CUPSIMAGE_LIBRARY}
    Threads::Threads
)
target_link_directories(pstoqpdl PRIVATE ${CUPS_LIBRARY_DIRS})

# ────────────────────────────────────────────────────────────────────────────
# Install: Filter Binaries
# ────────────────────────────────────────────────────────────────────────────
install(TARGETS rastertoqpdl pstoqpdl
    RUNTIME DESTINATION "${SPLIX_FILTER_DIR}"
)

# ────────────────────────────────────────────────────────────────────────────
# Install: PPD Files
# ────────────────────────────────────────────────────────────────────────────
# The PPD files for every supported printer model are pre-compiled and
# committed to the ppd/ directory.  We install them directly — no cups-ppdc
# runtime tool required.  This mirrors what the old ppd/Makefile did: it ran
# cups-ppdc at development time, committed the output, then the install target
# simply copied the files.
#
# Brand → CUPS sub-directory mapping (from ppd/Makefile):
#   SAMSUNG : clp clx m[0-9] ml scx sf  → samsung/
#   XEROX   : ph wc wcpe               → xerox/
#   DELL    : 1100 1110                → dell/
#   LEXMARK : x215                     → lexmark/
#   TOSHIBA : es                       → toshiba/
#
# HP (laser10x, laser13x) has no pre-built .ppd files in the repo; they are
# generated on-demand via cups-ppdc from hp.drv.in and not installed here.
#
# Language variants (French: *fr.ppd, Brazilian Portuguese: *pt.ppd) are
# included automatically by the *.ppd glob.

# Samsung ----------------------------------------------------------------
install(
    DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/ppd/"
    DESTINATION "${SPLIX_PPD_DIR}/samsung"
    FILES_MATCHING
        PATTERN "clp*.ppd"         # Colour laser
        PATTERN "clx*.ppd"         # Colour multifunction
        PATTERN "m[0-9]*.ppd"      # M-series (M2020, M2070, M262x …)
        PATTERN "ml*.ppd"          # Monochrome laser
        PATTERN "scx*.ppd"         # Monochrome multifunction
        PATTERN "sf*.ppd"          # Fax-capable multifunction
)

# Xerox ------------------------------------------------------------------
# Note: wcpe* is a subset of wc*, so a single wc*.ppd pattern covers both.
install(
    DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/ppd/"
    DESTINATION "${SPLIX_PPD_DIR}/xerox"
    FILES_MATCHING
        PATTERN "ph*.ppd"          # Phaser series
        PATTERN "wc*.ppd"          # WorkCentre series (wc* and wcpe*)
)

# Dell -------------------------------------------------------------------
install(
    DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/ppd/"
    DESTINATION "${SPLIX_PPD_DIR}/dell"
    FILES_MATCHING
        PATTERN "1100*.ppd"
        PATTERN "1110*.ppd"
)

# Lexmark ----------------------------------------------------------------
install(
    DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/ppd/"
    DESTINATION "${SPLIX_PPD_DIR}/lexmark"
    FILES_MATCHING
        PATTERN "x215*.ppd"
)

# Toshiba ----------------------------------------------------------------
install(
    DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/ppd/"
    DESTINATION "${SPLIX_PPD_DIR}/toshiba"
    FILES_MATCHING
        PATTERN "es*.ppd"
)

enable_testing()

# ── Google Test Integration ──────────────────────────────────────────────────
if(NOT CMAKE_CROSSCOMPILING)
    include(FetchContent)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG        v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    # Prevent GTest from installing its headers and static libraries into our release packages
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(INSTALL_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(splix_gtest tests/splix_gtest.cpp)
    target_link_libraries(splix_gtest PRIVATE 
        splix_core
        GTest::gtest_main
    )
    target_include_directories(splix_gtest PRIVATE src) # for access to internal headers if needed

    include(GoogleTest)
    gtest_discover_tests(splix_gtest)
endif()

# ── Status and Smoke Tests ───────────────────────────────────────────────────
# Smoke tests: verify the binaries exist and print their usage statement.
# Skipped automatically during cross-compilation (no emulator set).
if(NOT CMAKE_CROSSCOMPILING)
    add_test(
        NAME rastertoqpdl_usage
        COMMAND rastertoqpdl
    )
    set_tests_properties(rastertoqpdl_usage PROPERTIES
        PASS_REGULAR_EXPRESSION "Usage:"
    )

    add_test(
        NAME pstoqpdl_usage
        COMMAND pstoqpdl
    )
    set_tests_properties(pstoqpdl_usage PROPERTIES
        PASS_REGULAR_EXPRESSION "Usage:"
    )

    # ── Functional Integration Test ──────────────────────────────────────────
    # Simulates a real print job by feeding raster data to the filter.
    add_test(
        NAME driver_functional_raster
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/functional_test.sh" "${CMAKE_CURRENT_BINARY_DIR}"
    )
endif()

# ────────────────────────────────────────────────────────────────────────────
# CPack configuration (generates .deb packages)
# ────────────────────────────────────────────────────────────────────────────
include(cmake/SpliXCPack.cmake)
