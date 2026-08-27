cmake_minimum_required(VERSION 3.25)

project(splix_feature_probe LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../cmake")

include(SpliXFeatures)

if(NOT DEFINED SPLIX_REQUIRED_CXX_STANDARD)
    set(SPLIX_REQUIRED_CXX_STANDARD 20)
endif()
splix_require_cxx23()
