# cmake/SpliXCPack.cmake
#
# CPack configuration for generating .deb packages directly from CMake.
#
# Usage (after a successful build):
#   cd build && cpack          # generates splix-<version>-<arch>.deb
#
# Or via presets:
#   cpack --preset linux-amd64-release

# ── Package metadata ─────────────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME              "splix")
set(CPACK_PACKAGE_VERSION           "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION       "SpliX printer drivers for Samsung, Xerox, Dell, Lexmark, and Toshiba laser printers")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CUPS drivers for SPL/QPDL laser printers")
set(CPACK_PACKAGE_CONTACT           "github-actions@noreply.github.com")
set(CPACK_PACKAGE_HOMEPAGE_URL      "https://github.com/dutch2005/splix")
set(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")

# ── Generator settings ─────────────────────────────────────────────────────────
set(CPACK_GENERATOR                 "DEB;RPM")

# ── DEB-specific settings ────────────────────────────────────────────────────
set(CPACK_DEBIAN_PACKAGE_SECTION    "misc")
set(CPACK_DEBIAN_PACKAGE_PRIORITY   "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS    "cups")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS  ON)

# ── RPM-specific settings ────────────────────────────────────────────────────
set(CPACK_RPM_PACKAGE_REQUIRES      "cups")
set(CPACK_RPM_PACKAGE_LICENSE       "GPL-2.0")
set(CPACK_RPM_PACKAGE_GROUP         "System Environment/Daemons")

# ── Architecture detection ───────────────────────────────────────────────────
# When cross-compiling, CMAKE_SYSTEM_PROCESSOR is set by the toolchain file
# (e.g. "aarch64").  Map it to the Debian architecture name.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
else()
    # Let dpkg-architecture figure it out
    execute_process(
        COMMAND dpkg --print-architecture
        OUTPUT_VARIABLE _dpkg_arch
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(_dpkg_arch)
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${_dpkg_arch}")
    else()
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    endif()
endif()

# ── Output file naming ───────────────────────────────────────────────────────
# Produces: splix-2.0.2-arm64.deb  or  splix-2.0.2-amd64.deb
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")

include(CPack)
