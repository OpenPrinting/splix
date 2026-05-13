# SpliX Changelog

All notable changes to this project will be documented in this file.

## [2.0.2] - 2026-04-24

This release represents a comprehensive modernization of the SpliX driver. The primary goal of this update is to transition the 2006-era C++98 codebase to modern C++23 standards, ensuring memory safety, thread safety, and long-term maintainability, while maintaining 100% bit-perfect protocol compatibility with all supported Samsung, Xerox, and Dell printers.

### Added

- **Multi-Architecture Build System**: Completely replaced the legacy `Makefile` system with a modern `CMake` (3.25+) build system.
- **Automated Packaging**: Integrated `CPack` to automatically generate both `.deb` (Debian/Ubuntu) and `.rpm` (Fedora/RHEL) packages for AMD64 and ARM64 architectures.
- **Testing Infrastructure**: Introduced Google Test (GTest) framework with a comprehensive suite of unit tests validating critical QPDL compression algorithms (`0x11`, `0x15`, `0x0D`, `0x0E`) against known-good byte-stream outputs.
- **Security Hardening**: Build pipeline now enforces modern compiler security flags by default: Full RELRO, PIE, Stack Protection (`-fstack-protector-strong`), and Fortify Source (`-D_FORTIFY_SOURCE=2`).
- **Buffer Safety Guards**: Added explicit output-size capacity checks to the LZS (Algo 0x11) compression algorithms to prevent edge-case buffer overflows that could historically crash printer firmware.
- **New Hardware Support**: Integrated support and pre-compiled PPDs for the **Samsung ML-1670** and **Samsung SCX-3400** printers.

### Changed

- **Memory Management Modernization**: Eliminated all manual memory management (`malloc`, `free`, `new`, `delete`) and raw pointer arithmetic. Replaced with `std::vector`, `std::unique_ptr`, and `std::span` to guarantee memory safety and eliminate leaks.
- **Thread Synchronization**: Replaced legacy, platform-dependent POSIX threading and custom semaphore implementations with standard C++ concurrency primitives (`std::mutex`, `std::counting_semaphore`, `std::jthread`).
- **Endianness Handling**: Replaced duplicated `#ifdef WORDS_BIGENDIAN` preprocessor blocks with modern, standard-compliant `memcpy` and native type handling. This simplifies the code while guaranteeing correct Little-Endian payload generation for the printer, regardless of the host CPU architecture (e.g., x86 vs ARM vs PowerPC).
- **Error Handling**: Transitioned from generic boolean/integer return codes to structured C++ error handling (using Result/Expected patterns via `<expected>`) to provide granular visibility into pipeline failures.
- **Pre-compiled PPDs**: The build process now generates and ships all 248 `.ppd` files natively, eliminating the requirement for end-users to have `cups-ppdc` installed on modern systems.

### Fixed

- Fixed various `-Wdeprecated-declarations` and scope warnings when compiling with GCC 13+ and Clang 16+.
- Fixed cross-platform line-ending discrepancies (`\r\n` vs `\n`) in bash scripts by enforcing LF via `.gitattributes`.
- Addressed minor alignment issues in `renderPage` PJL headers for specific color models (e.g., CLP-315) to guarantee firmware synchronization.

### Removed

- Removed the deprecated legacy `Makefile` and `rules.mk` files in favor of `CMakeLists.txt`.
- Removed duplicated buffer-tracking boilerplate code across the `Page`, `Band`, and `Document` classes, as standard C++ containers now manage capacities natively.

---
*Note to maintainers: This modernization was executed with strict adherence to the QPDL/SPL protocol. The byte-stream output has been extensively regression-tested against the legacy driver outputs to guarantee 100% hardware compatibility.*
