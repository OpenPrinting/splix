# SpliX Changelog

All notable changes to this project will be documented in this file.

## [3.0.0] - 2026-08-27

This release proposal rebases Michael Maertzdorf's (`dutch2005`) comprehensive modernization onto OpenPrinting's 2.0.2 code line. It moves the 2006-era C++98 implementation to C++23 while preserving the SPL/QPDL protocol behavior covered by characterization and functional tests.

### Added

- **Multi-Architecture Build System**: Completely replaced the legacy `Makefile` system with a modern `CMake` (3.25+) build system.
- **Automated Packaging**: Integrated `CPack`; CI produces `.deb` packages for AMD64, ARM64, ARMHF, i386, and RISC-V 64.
- **Testing Infrastructure**: Introduced Google Test (GTest) characterization for critical QPDL compression algorithms (`0x11`, `0x15`, `0x0D`, `0x0E`) plus functional filter tests.
- **Security Hardening**: Build pipeline now enforces modern compiler security flags by default: Full RELRO, PIE, Stack Protection (`-fstack-protector-strong`), and Fortify Source (`-D_FORTIFY_SOURCE=3`).
- **Buffer Safety Guards**: Added explicit output-size capacity checks to the LZS (Algo 0x11) compression algorithms to prevent edge-case buffer overflows that could historically crash printer firmware.
- **New Hardware Support**: Integrated support and pre-compiled PPDs for the **Samsung ML-1670** and **Samsung SCX-3400** printers.

### Changed

- **Memory Management Modernization**: Migrated ownership-heavy paths to `std::vector`, `std::unique_ptr`, and `std::span`, reducing manual lifetime and bounds management.
- **Thread Synchronization**: Replaced legacy cache handoffs and platform-dependent primitives with `std::mutex`, `std::condition_variable`, `std::counting_semaphore`, and `std::jthread`.
- **Endianness Handling**: Replaced duplicated `#ifdef WORDS_BIGENDIAN` preprocessor blocks with modern, standard-compliant `memcpy` and native type handling. This simplifies the code while guaranteeing correct Little-Endian payload generation for the printer, regardless of the host CPU architecture (e.g., x86 vs ARM vs PowerPC).
- **Error Handling**: Transitioned from generic boolean/integer return codes to structured C++ error handling (using Result/Expected patterns via `<expected>`) to provide granular visibility into pipeline failures.
- **Pre-compiled PPDs**: The build process now generates and ships all 248 `.ppd` files natively, eliminating the requirement for end-users to have `cups-ppdc` installed on modern systems.

### Fixed

- Added a configure-time probe that rejects compiler/standard-library pairs missing the required C++23 facilities.
- Replaced the racy cache-controller handoff that could stall when the 30-page
  resident limit was crossed; regression coverage now exercises 64-page and
  manual-duplex ordering.
- Fixed cross-platform line-ending discrepancies (`\r\n` vs `\n`) in bash scripts by enforcing LF via `.gitattributes`.
- Addressed minor alignment issues in `renderPage` PJL headers for specific color models (e.g., CLP-315) to guarantee firmware synchronization.

### Removed

- Removed the deprecated legacy `Makefile` and `rules.mk` files in favor of `CMakeLists.txt`.
- Removed duplicated buffer-tracking boilerplate code across the `Page`, `Band`, and `Document` classes, as standard C++ containers now manage capacities natively.

---
*Note to maintainers: Protocol compatibility is guarded by byte-level characterization, functional filter tests, sanitizer runs, and multi-architecture builds. Physical-printer testing remains valuable before a final release.*
