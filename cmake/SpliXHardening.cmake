# cmake/SpliXHardening.cmake
#
# Senior-level security hardening for SpliX (Linux CUPS Driver).
# These flags are designed to protect the driver against common memory
# corruption and injection attacks, crucial for system-level filters.

include(CheckCXXCompilerFlag)

macro(splix_apply_hardening target)
    message(STATUS "Applying specialist hardening to target: ${target}")

    # ── Compiler Flags ───────────────────────────────────────────────────────
    
    # 1. Stack Protection: Detects stack smashing.
    #    -fstack-protector-strong provides a balance between coverage and perf.
    target_compile_options(${target} PRIVATE -fstack-protector-strong)

    # 2. Buffer Fortification: Adds checks for dangerous functions (memcpy, etc).
    #    Level 3 is the modern gold standard. Requires optimization.
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${target} PRIVATE _FORTIFY_SOURCE=3)
    endif()

    # 3. Format Security: Prevents %n and related string vulnerabilities.
    target_compile_options(${target} PRIVATE -Wformat -Werror=format-security)

    # 4. Strict Warnings: Catch potential bugs at compile time.
    target_compile_options(${target} PRIVATE 
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wcast-align 
        -Wunused -Woverloaded-virtual -Wold-style-cast -Wnon-virtual-dtor
    )

    # 5. Runtime STL Assertions: Catches out-of-bounds array access in std containers.
    target_compile_definitions(${target} PRIVATE _GLIBCXX_ASSERTIONS)

    # 6. Prevent common global collisions and stack clash protection
    target_compile_options(${target} PRIVATE -fno-common -fstack-clash-protection)

    # 6. Position Independent Executable (PIE): Required for modern ASLR.
    set_target_properties(${target} PROPERTIES 
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        POSITION_INDEPENDENT_CODE ON
    )

    # ── Linker Flags ─────────────────────────────────────────────────────────

    # 7. RELRO (Read-Only Relocations): Makes the Global Offset Table read-only.
    #    "now" (Full RELRO) ensures all symbols are resolved at startup.
    if(NOT APPLE)
        target_link_options(${target} PRIVATE "LINKER:-z,relro" "LINKER:-z,now")
    endif()

    # 8. No-Executable Stack: Prevents execution from stack memory.
    if(NOT APPLE)
        target_link_options(${target} PRIVATE "LINKER:-z,noexecstack")
    endif()

endmacro()

# ── Link Time Optimization (LTO) ─────────────────────────────────────────────
# Optimizes code across translation units. Significant for raster processing.
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _LTO_SUPPORTED OUTPUT _LTO_ERROR)
    if(_LTO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "LTO (Interprocedural Optimization) enabled for Release.")
    else()
        message(STATUS "LTO not supported: ${_LTO_ERROR}")
    endif()
endif()
