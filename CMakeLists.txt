cmake_minimum_required(VERSION 3.25)

project(splix
    VERSION 3.0.0
    DESCRIPTION "Samsung, Xerox, and Dell QPDL printer driver"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(SpliXFeatures)
splix_require_cxx23()

include(SpliXOptions)
include(SpliXHardening)
include(SpliXDependencies)
include(SpliXTargets)
include(SpliXInstall)
include(SpliXTests)
include(SpliXCPack)
