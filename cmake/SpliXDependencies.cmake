find_package(PkgConfig REQUIRED)
find_package(Cups)

if(NOT CUPS_FOUND)
    message(FATAL_ERROR
        "[splix] CUPS development headers not found. "
        "Install cups and libcups2-dev.")
endif()

find_library(CUPSIMAGE_LIBRARY
    NAMES cupsimage
    HINTS ${CUPS_LIBRARY_DIRS}
    DOC "CUPS image conversion library")
if(NOT CUPSIMAGE_LIBRARY)
    message(FATAL_ERROR
        "[splix] libcupsimage not found. Install libcupsimage2-dev.")
endif()

if(NOT DISABLE_JBIG)
    find_library(JBIG_LIBRARY
        NAMES jbig
        DOC "JBIG compression library")
    if(NOT JBIG_LIBRARY)
        message(FATAL_ERROR
            "[splix] libjbig not found. Install libjbig-dev.")
    endif()
endif()

find_package(Threads REQUIRED)

find_program(CUPS_CONFIG_EXECUTABLE NAMES cups-config)
if(CUPS_CONFIG_EXECUTABLE)
    execute_process(
        COMMAND ${CUPS_CONFIG_EXECUTABLE} --serverbin
        OUTPUT_VARIABLE _CUPS_SERVERBIN
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(
        COMMAND ${CUPS_CONFIG_EXECUTABLE} --datadir
        OUTPUT_VARIABLE _CUPS_DATADIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    execute_process(
        COMMAND pkg-config --variable=cups_serverbin cups
        OUTPUT_VARIABLE _CUPS_SERVERBIN
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(
        COMMAND pkg-config --variable=cups_datadir cups
        OUTPUT_VARIABLE _CUPS_DATADIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

if(NOT _CUPS_SERVERBIN)
    set(_CUPS_SERVERBIN "/usr/lib/cups")
endif()
if(NOT _CUPS_DATADIR)
    set(_CUPS_DATADIR "/usr/share/cups")
endif()

set(SPLIX_FILTER_DIR "${_CUPS_SERVERBIN}/filter"
    CACHE STRING "CUPS filter directory")
set(SPLIX_PPD_DIR "${_CUPS_DATADIR}/model"
    CACHE STRING "CUPS PPD directory")
set(SPLIX_DRV_DIR "${_CUPS_DATADIR}/drv"
    CACHE STRING "CUPS driver-source directory")
set(SPLIX_PROFILE_DIR "${_CUPS_DATADIR}/profiles"
    CACHE STRING "CUPS color-profile directory")
set(PSTORASTER_BIN "pstoraster"
    CACHE STRING "PostScript-to-raster filter")
set(GSTORASTER_BIN "gstoraster"
    CACHE STRING "Ghostscript-to-raster filter")

message(STATUS "SpliX filter directory: ${SPLIX_FILTER_DIR}")
message(STATUS "SpliX PPD directory: ${SPLIX_PPD_DIR}")
message(STATUS "SpliX profile directory: ${SPLIX_PROFILE_DIR}")
