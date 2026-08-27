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
set(CPACK_PACKAGE_HOMEPAGE_URL      "https://github.com/OpenPrinting/splix")
set(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")

# ── Generator settings ─────────────────────────────────────────────────────────
set(CPACK_GENERATOR                 "DEB")

# ── DEB-specific settings ────────────────────────────────────────────────────
set(CPACK_DEBIAN_PACKAGE_SECTION    "misc")
set(CPACK_DEBIAN_PACKAGE_PRIORITY   "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS    "cups")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS  ON)

# ── Architecture detection ───────────────────────────────────────────────────
# Cross builds must use the target processor, never the build host returned by
# dpkg. Keep the mapping pure so it can be tested without a compiler.
include(SpliXArchitecture)
splix_debian_architecture(
    CPACK_DEBIAN_PACKAGE_ARCHITECTURE
    "${CMAKE_SYSTEM_PROCESSOR}")

# ── Output file naming ───────────────────────────────────────────────────────
# Produces: splix-3.0.0-arm64.deb  or  splix-3.0.0-amd64.deb
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")

include(CPack)
