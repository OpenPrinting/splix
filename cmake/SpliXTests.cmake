include(CTest)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(INSTALL_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    function(splix_add_gtest target)
        add_executable(${target} ${ARGN})
        splix_apply_hardening(${target})
        target_link_libraries(${target} PRIVATE
            splix_core
            GTest::gtest_main)
        target_include_directories(${target} PRIVATE src)
        gtest_discover_tests(${target} PROPERTIES TIMEOUT 30)
    endfunction()

    include(GoogleTest)
    splix_add_gtest(splix_gtest
        tests/compression_characterization_test.cpp
        tests/splix_gtest.cpp)
    splix_add_gtest(protocol_characterization_test
        tests/protocol_characterization_test.cpp)
    if(NOT DISABLE_THREADS)
        splix_add_gtest(cache_concurrency_test
            tests/cache_concurrency_test.cpp)
    endif()

    add_test(
        NAME package_identity
        COMMAND ${CMAKE_COMMAND}
            -DSPLIX_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/package_identity_test.cmake)

    add_test(NAME rastertoqpdl_usage COMMAND rastertoqpdl)
    set_tests_properties(rastertoqpdl_usage PROPERTIES
        PASS_REGULAR_EXPRESSION "Usage:")

    add_test(NAME pstoqpdl_usage COMMAND pstoqpdl)
    set_tests_properties(pstoqpdl_usage PROPERTIES
        PASS_REGULAR_EXPRESSION "Usage:")

    add_test(
        NAME driver_functional_raster
        COMMAND bash
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/functional_test.sh"
            "${CMAKE_CURRENT_BINARY_DIR}")
endif()
