option(DISABLE_JBIG "Disable JBIG compression" OFF)
option(DISABLE_THREADS "Disable threading" OFF)
option(DISABLE_BLACKOPTIM "Disable black optimization" OFF)

set(THREADS "2" CACHE STRING "Number of rendering threads")
set(CACHESIZE "30" CACHE STRING "Maximum number of resident compressed pages")

find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY
        ${CLANG_TIDY_EXE}
        -checks=-*,readability-*,bugprone-*,modernize-*,cppcoreguidelines-,-cppcoreguidelines-pro-type-member-init,-cppcoreguidelines-pro-type-static-cast-downcast)
    message(STATUS "Clang-Tidy enabled")
endif()

find_program(CPPCHECK_EXE NAMES cppcheck)
if(CPPCHECK_EXE)
    set(CMAKE_CXX_CPPCHECK
        ${CPPCHECK_EXE}
        --enable=warning,performance,portability,style
        --inconclusive
        --force)
    message(STATUS "Cppcheck enabled")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/version.h"
    @ONLY)
