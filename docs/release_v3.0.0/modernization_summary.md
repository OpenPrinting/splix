# SpliX v3.0.0: Modernization & Stabilization Summary

This document summarizes the C++23 modernization proposed for the `v3.0.0` release of the SpliX printer driver. The rework was contributed by Michael Maertzdorf (`dutch2005`) and rebased onto OpenPrinting's 2.0.2 code line.

## Key Improvements

### 1. C++23 Core Refactor

Core ownership, error-handling, compression, and synchronization paths now use
C++23 facilities while preserving the existing SPL/QPDL output contracts.

- **`std::span` Adoption**: Used across all compression and rasterization layers to eliminate pointer arithmetic and ensure safe buffer access.
- **RAII Patterns**: Legacy manual memory management (`malloc`/`free`) has been replaced with standard containers and scope-based resource management.
- **Thread Safety**: Synchronization was overhauled using `std::semaphore` and `std::mutex`, replacing legacy platform-specific primitives.

### 2. Reviewable Source Layout

Large implementation units are split into responsibility-focused include
fragments while remaining single translation units. This keeps the established
linkage and behavior intact and makes the core, protocol, rendering, and tool
paths easier to review.

### 3. Build & CI Infrastructure

- **CMake Conversion**: The legacy Makefile system was replaced with a modern CMake configuration, providing better dependency management and IDE integration.
- **Multi-Architecture Support**: Automated `.deb` packaging for `amd64`, `arm64`, `armhf`, `i386`, and `riscv64`.
- **Security Hardening**: Build flags now include Full RELRO, PIE, Stack Protection, and Fortify Source.

### 4. Verification & Testing

- **GTest Integration**: Critical compression algorithms are now covered by unit tests.
- **Sanitizers**: Continuous Integration (CI) now runs AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) on every pull request.

## For Future Developers

Before implementing new features:

1. Ensure your environment supports **C++23**.
2. Configure and build with a documented CMake preset.
3. Run `ctest --output-on-failure` and add tests for changed behavior.

---
Prepared: August 27, 2026
