if(NOT DEFINED SPLIX_SOURCE_DIR)
    get_filename_component(SPLIX_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

include("${SPLIX_SOURCE_DIR}/cmake/SpliXArchitecture.cmake")

function(expect_debian_arch processor expected)
    splix_debian_architecture(actual "${processor}")
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR
            "Processor ${processor} mapped to ${actual}; expected ${expected}")
    endif()
endfunction()

expect_debian_arch("x86_64" "amd64")
expect_debian_arch("aarch64" "arm64")
expect_debian_arch("armv7l" "armhf")
expect_debian_arch("i686" "i386")
expect_debian_arch("riscv64" "riscv64")

file(READ "${SPLIX_SOURCE_DIR}/CMakeLists.txt" root_cmake)
if(NOT root_cmake MATCHES "VERSION 3\\.0\\.0")
    message(FATAL_ERROR "CMake project version is not 3.0.0")
endif()

file(GLOB ppd_files "${SPLIX_SOURCE_DIR}/ppd/*.ppd")
list(LENGTH ppd_files ppd_count)
if(NOT ppd_count EQUAL 248)
    message(FATAL_ERROR "Expected 248 generated PPD files, found ${ppd_count}")
endif()

foreach(ppd_file IN LISTS ppd_files)
    file(READ "${ppd_file}" ppd_contents)
    string(FIND "${ppd_contents}" "*FileVersion: \"3.0.0\"" version_offset)
    if(version_offset EQUAL -1)
        message(FATAL_ERROR "${ppd_file} does not identify version 3.0.0")
    endif()
endforeach()

file(GLOB legacy_root_ppds "${SPLIX_SOURCE_DIR}/*.ppd")
if(legacy_root_ppds)
    message(FATAL_ERROR
        "Legacy root-level PPD duplicates remain: ${legacy_root_ppds}")
endif()

if(EXISTS "${SPLIX_SOURCE_DIR}/include/version.h")
    message(FATAL_ERROR
        "Static include/version.h shadows the generated 3.0.0 header")
endif()
