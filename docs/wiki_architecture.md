# SpliX Architecture Guide

SpliX is a CUPS filter for Samsung SPL2/SPLc/QPDL-compatible printers. The
printer protocol is byte-oriented, so changes to rendering and compression
must preserve the characterized output snapshots in `tests/`.

## Filter entry points

- `src/rastertoqpdl.cpp` is the main CUPS raster filter. It loads the PPD and
  request, initializes the page cache, calls `render()`, and writes QPDL to
  standard output.
- `src/pstoqpdl.cpp` is the PostScript wrapper. It invokes the configured
  Ghostscript/CUPS rasterizer and then `rastertoqpdl`.

`src/core.cpp` is retained as an empty compatibility translation unit; it does
not orchestrate the print job.

## Job and page pipeline

- `src/request.cpp` reads job options and owns the selected `Printer` state.
- `src/document.cpp` opens the CUPS raster stream and creates numbered `Page`
  instances.
- `src/page.cpp` owns page metadata, color planes, compressed bands, and the
  serialization used when pages are temporarily swapped to disk.
- `src/band.cpp` and `src/bandplane.cpp` hold compressed bands and color-plane
  payloads.

With threading enabled, `src/rendering.cpp` starts compression workers. Each
worker obtains a raw page from the shared `Document`, compresses it, and
registers it in the cache. The output path consumes pages in simplex order or
manual-duplex order.

## Cache and concurrency

`src/cache.cpp` is split into responsibility-focused `.inc` files that compile
as one translation unit:

- `cache_state.inc` owns the mutex, condition variable, page table, policy, and
  resident-page accounting.
- `cache_entry.inc` swaps compressed pages to disk and restores them.
- `cache_api.inc` implements initialization, page registration, ordered page
  delivery, duplex policy selection, and shutdown.

`CACHESIZE` is the maximum number of compressed pages resident in memory. Extra
pages are swapped to temporary files and restored synchronously when selected.

## Compression and protocol output

- `src/compress.cpp` selects whole-page or banded compression based on printer
  capabilities.
- `src/algo0x0d.cpp`, `algo0x0e.cpp`, and `algo0x11.cpp` implement Samsung
  compression variants.
- `src/algo0x13.cpp` and `algo0x15.cpp` use libjbig.
- `src/qpdl.cpp` writes page and band headers plus compressed payloads.
- `src/printer.cpp` writes the PJL job header and footer around QPDL output.

The larger implementations use `.inc` fragments to remain reviewable without
changing translation-unit boundaries or internal linkage.

## Build and verification

The root `CMakeLists.txt` delegates dependencies, targets, install rules,
hardening, tests, architecture mapping, and packaging to `cmake/`. CI builds
Debian packages for amd64, arm64, armhf, i386, and riscv64, and runs the native
test suite under AddressSanitizer and UndefinedBehaviorSanitizer.

Before changing protocol or concurrency code, run the tests described in
`BUILDING.md`. Physical-printer testing remains valuable for a release because
automated tests cannot validate every firmware variant.
