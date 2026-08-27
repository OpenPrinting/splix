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
    src/sp_semaphore.cpp)

add_library(splix_core STATIC ${SPLIX_CORE_SOURCES})
target_compile_features(splix_core PUBLIC cxx_std_23)
splix_apply_hardening(splix_core)
target_link_libraries(splix_core PUBLIC Cups::Cups Threads::Threads)
target_include_directories(splix_core PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}/include"
    "${CUPS_INCLUDE_DIR}"
    "${CUPS_INCLUDE_DIRS}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_options(splix_core PUBLIC ${CUPS_CFLAGS_OTHER})

if(DISABLE_THREADS)
    target_compile_definitions(splix_core PUBLIC DISABLE_THREADS)
else()
    target_compile_definitions(splix_core PUBLIC
        THREADS=${THREADS}
        CACHESIZE=${CACHESIZE})
endif()

if(DISABLE_JBIG)
    target_compile_definitions(splix_core PUBLIC DISABLE_JBIG)
else()
    # Algo0x13 and Algo0x15 live in the core archive. Consumers that exercise
    # those objects, including characterization tests, need the JBIG symbols.
    target_link_libraries(splix_core PUBLIC ${JBIG_LIBRARY})
endif()
if(DISABLE_BLACKOPTIM)
    target_compile_definitions(splix_core PUBLIC DISABLE_BLACKOPTIM)
endif()

add_executable(rastertoqpdl src/rastertoqpdl.cpp)
splix_apply_hardening(rastertoqpdl)
target_link_libraries(rastertoqpdl PRIVATE
    splix_core
    ${CUPS_LIBRARIES}
    ${CUPSIMAGE_LIBRARY}
    Threads::Threads)
target_link_directories(rastertoqpdl PRIVATE ${CUPS_LIBRARY_DIRS})

add_executable(pstoqpdl src/pstoqpdl.cpp)
splix_apply_hardening(pstoqpdl)
target_compile_definitions(pstoqpdl PRIVATE
    RASTERDIR="${SPLIX_FILTER_DIR}"
    RASTERTOQPDL="rastertoqpdl"
    GSTORASTER="${GSTORASTER_BIN}"
    PSTORASTER="${PSTORASTER_BIN}"
    CUPSPPD="${SPLIX_PPD_DIR}"
    CUPSPROFILE="${SPLIX_PROFILE_DIR}")
target_link_libraries(pstoqpdl PRIVATE
    splix_core
    ${CUPS_LIBRARIES}
    ${CUPSIMAGE_LIBRARY}
    Threads::Threads)
target_link_directories(pstoqpdl PRIVATE ${CUPS_LIBRARY_DIRS})
