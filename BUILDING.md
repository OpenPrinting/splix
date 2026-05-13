# Building SpliX

This document explains the build system, what changed from the original, and
how to build SpliX locally or understand what the CI pipeline does.

---

## Table of Contents

1. [Quick Start (Local Build)](#quick-start)
2. [Cross-Compilation (ARM64)](#cross-compilation-arm64)
3. [What Changed and Why](#what-changed-and-why)
4. [File-by-File Change Log](#file-by-file-change-log)
5. [CMake Variable Reference](#cmake-variable-reference)
6. [CI / GitHub Actions](#ci--github-actions)
7. [PPD File Map (Brand → Directory)](#ppd-file-map)
8. [Troubleshooting](#troubleshooting)

---

## Quick Start

### Prerequisites

```bash
# Debian / Ubuntu
sudo apt-get install \
    build-essential cmake pkg-config \
    cups \           # runtime: creates /usr/lib/cups/filter/, needed for path queries
    libcups2-dev \   # build: cups/cups.h headers + cups.pc for pkg-config
    libcupsimage2-dev \
    libjbig-dev
```


### Build

```bash
# Out-of-tree build (recommended — keeps the source tree clean)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Using CMake Presets (recommended)

The project includes `CMakePresets.json` with ready-made configurations.
Presets require CMake 3.25+ but the project itself builds with CMake 3.16+.

```bash
# AMD64 release build
cmake --preset linux-amd64-release
cmake --build --preset linux-amd64-release

# ARM64 cross-compiled release build
cmake --preset linux-arm64-release
cmake --build --preset linux-arm64-release

# Debug build for development
cmake --preset linux-debug
cmake --build --preset linux-debug
```

### Install

```bash
# System-wide (requires root)
sudo cmake --install build

# Staging directory (for packaging)
cmake --install build --destdir /tmp/splix-stage
```

### Override install paths

```bash
cmake -S . -B build \
    -DSPLIX_FILTER_DIR=/usr/lib/cups/filter \
    -DSPLIX_PPD_DIR=/usr/share/cups/model
```

### Generating `.deb` with CPack

CPack is configured automatically. After building:

```bash
# Via presets:
cpack --preset linux-amd64-release

# Or manually:
cd build && cpack -G DEB
```

The output is `splix-2.0.2-amd64.deb` (or `arm64`).

---

## Cross-Compilation (ARM64)

CMake supports cross-compilation natively via **toolchain files**.  A toolchain
file tells CMake which compiler to use and where the target system's libraries
live.  The cross-compiler builds ARM64 binaries directly on the x86-64 host —
no QEMU emulation required.

**Why this matters:** QEMU emulation of ARM64 on x86-64 takes 15-25 minutes
for a cold build.  Native cross-compilation takes 2-5 minutes.

### Prerequisites (install once)

```bash
# Enable ARM64 multiarch so apt can install :arm64 packages
sudo dpkg --add-architecture arm64
sudo apt-get update

# Cross-compiler and ARM64 development libraries
sudo apt-get install \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
    libcups2-dev:arm64 libcupsimage2-dev:arm64 libjbig-dev:arm64
```

### Build for ARM64

```bash
cmake -S . -B build-arm64 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm64.cmake \
      -DCMAKE_BUILD_TYPE=Release
cmake --build build-arm64 --parallel
```

### How `cmake/toolchain-arm64.cmake` works

| Setting | Value | Purpose |
|---|---|---|
| `CMAKE_C_COMPILER` | `aarch64-linux-gnu-gcc` | Cross-compiler binary |
| `CMAKE_CXX_COMPILER` | `aarch64-linux-gnu-g++` | Cross-compiler binary |
| `CMAKE_FIND_ROOT_PATH` | `/usr/lib/aarch64-linux-gnu` | Where to find ARM64 libraries |
| `CMAKE_FIND_ROOT_PATH_MODE_LIBRARY` | `ONLY` | Never fall back to host libraries |
| `PKG_CONFIG_LIBDIR` (env) | `/usr/lib/aarch64-linux-gnu/pkgconfig` | Makes pkg-config query ARM64 `.pc` files |

The toolchain file is processed **before** `CMakeLists.txt`, so the
`PKG_CONFIG_LIBDIR` override is in effect when `pkg_check_modules(CUPS ...)`
and the `execute_process(pkg-config --variable=...)` calls run.  CUPS install
paths (`/usr/lib/cups/filter`, `/usr/share/cups/model`) are architecture-
independent, so the returned values are correct for both amd64 and arm64.

---


### Old build system: the legacy `Makefile`

SpliX was written in 2006–2008. The original build system is a hand-rolled
recursive Make framework spread across:

| File | Purpose |
|---|---|
| `Makefile` | Root orchestrator — 390 lines of macro-heavy GNU Make |
| `module.mk` | Project-level flags, targets, library names |
| `src/module.mk` | Source file lists for each binary |
| `rules.mk` | Link rules, install rules, `drv`/`ppd` targets |
| `ppd/Makefile` | PPD file compilation and installation (all 5 brands) |

**Problems with the old system (as of 2025):**

1. **Hard-coded library name `-ljbig85`** — the Knoppix-era name for the JBIG
   library. Modern Debian and Ubuntu ship it as `-ljbig`. Every CI attempt
   had to patch this at runtime with `sed`.

2. **Hard-coded macOS paths** — `-I/opt/local/include` and `-L/opt/local/lib`
   (Homebrew/MacPorts paths) were unconditionally added to Linux builds,
   cluttering the compiler command and confusing cl-cache tools.

3. **`pkg-config` invoked inside recipe strings** — paths like `CUPSFILTER`
   were resolved by running backtick sub-shells inside Makefile variable
   assignments, making cross-compilation and reproducible builds fragile.

4. **No out-of-tree build support** — running `make` always wrote object files
   into the source tree, making it impossible to build multiple configurations
   simultaneously.

5. **Packaging (`checkinstall`) broke consistently** — the `cp *.deb` glob
   at the end of the workflow expanded in the wrong shell context, producing
   silent failures with no artifact uploaded.

### New build system: `CMakeLists.txt`

CMake replaces all five files above with a single, self-contained
`CMakeLists.txt`. It uses standard CMake idioms that package maintainers and
IDE tooling understand natively.

**Key improvements:**

| Concern | Old Makefile | New CMake |
|---|---|---|
| Library name | `-ljbig85` (hardcoded, wrong) | `find_library(JBIG_LIBRARY NAMES jbig)` |
| `libcupsimage` | Implicit via `pkg-config --libs cups` | Explicit `find_library(CUPSIMAGE_LIBRARY ...)` |
| CUPS paths | `pkg-config` backtick inside recipe | `execute_process()` at configure time → `CACHE STRING` |
| pstoqpdl defines | Six `-D` flags in `module.mk` | `target_compile_definitions(pstoqpdl PRIVATE ...)` |
| Out-of-tree build | Not supported | Native (`cmake -B build`) |
| Override paths | Edit `module.mk` | `-DSPLIX_FILTER_DIR=...` on cmake command line |
| Cross-arch | Fragile env-var hacks | CMake's toolchain abstraction |

---

## File-by-File Change Log

### `module.mk` — 3 lines changed

| Line | Before | After | Reason |
|---|---|---|---|
| 31 | `-I/opt/local/include` | `-std=c++11` | Remove dead macOS Homebrew path; pin C++ standard |
| 34 | `$(LDFLAGS) -L/opt/local/lib` | `$(LDFLAGS)` | Remove dead macOS Homebrew lib path |
| 51 | `-ljbig85` | `-ljbig` | Fix the library name to match modern Debian/Ubuntu packaging |

> **Note:** `module.mk` is kept for users who still want to build with `make`.
> The CMake build does not use it — it detects everything from scratch.

---

### `CMakeLists.txt` — new file

Complete replacement for the Makefile build system. See the inline comments
in the file for rationale on every decision. Key sections:

| CMake section | Replaces |
|---|---|
| `find_package(CUPS)` + `find_library(CUPSIMAGE_LIBRARY)` | `pkg-config --libs cups` + `-lcupsimage` in `module.mk` |
| `find_library(JBIG_LIBRARY)` | `-ljbig85` (patched to `-ljbig`) in `module.mk` |
| `execute_process(pkg-config --variable=...)` | Backtick expansions in `module.mk` for paths |
| `add_library(splix_core STATIC ...)` | Implicit object list from `src/module.mk` |
| `target_compile_definitions(pstoqpdl PRIVATE ...)` | `src_pstoqpdl_cpp_FLAGS` in `module.mk` |
| `install(TARGETS ...)` | `install:` target in `rules.mk` |
| `install(DIRECTORY ppd/ ...)` | `install:` target in `ppd/Makefile` (all 5 brands) |

---

### `.github/workflows/Build.yml` — rewritten

The repo previously contained **three** competing workflow files, all with the
same structural bugs:

| Old file | Status |
|---|---|
| `Build.yml` | Retained and rewritten |
| `Build Splix Drivers V3 (Clean Detection).yml` | **Deleted** — duplicate, same bugs |
| `build-works-but-not-complete.yml` | **Deleted** — incomplete, same bugs |

**Bugs fixed in the new workflow:**

| Bug | Impact | Fix |
|---|---|---|
| `cp *.deb` glob expansion | `.deb` never copied to artifacts → no artifact uploaded | Replaced with `find -name "*.deb" -exec cp` |
| `checkinstall` + wrong working directory | `checkinstall` wrote `.deb` into source dir, `cp` looked in build dir | Build in `/tmp/splix-build`; `cd` there before `checkinstall`; `find` in same dir |
| `rastertoqpdl_LIBS` env var + `sed` patch both active | Double-patching caused linker failures on some runs | Removed env var overrides; library name fixed in source (`module.mk`) |
| `pkg_config_arch` defined but unused | Silent mis-configuration | Variable removed; CMake handles this automatically |
| `fail-fast: true` (default) | ARM64 failure killed AMD64 build and vice versa | `fail-fast: false` added to matrix |
| No artifact failure detection | Workflow showed green even with no `.deb` | `if-no-files-found: error` on upload step |
| No release support | Manual process required to attach `.deb` to a tag | `softprops/action-gh-release` step on `refs/tags/v*` |
| `ccache` not wired to CMake | CMake bypassed `PATH`-based wrappers | `-DCMAKE_CXX_COMPILER_LAUNCHER=ccache` flag |

---

## CMake Variable Reference

All variables can be overridden on the `cmake` command line with `-D`.

| Variable | Default (from pkg-config) | Description |
|---|---|---|
| `SPLIX_FILTER_DIR` | `$(cups_serverbin)/filter` | Where CUPS filter binaries are installed |
| `SPLIX_PPD_DIR` | `$(cups_datadir)/model` | Where PPD driver files are installed |
| `SPLIX_DRV_DIR` | `$(cups_datadir)/drv` | Where DRV source files are installed |
| `SPLIX_PROFILE_DIR` | `$(cups_datadir)/profiles` | Where colour profiles are installed |
| `PSTORASTER_BIN` | `pstoraster` | Runtime name of the PS-to-raster filter |
| `GSTORASTER_BIN` | `gstoraster` | Runtime name of the GS-to-raster filter |

---

## CI / GitHub Actions

The single workflow file (`.github/workflows/Build.yml`) builds inside a
`debian:oldstable` Docker container for maximum binary compatibility with
older Debian and Ubuntu systems.

### Matrix

| Arch | Runner | Compilation method |
|---|---|---|
| `amd64` | `ubuntu-latest` (x86-64) | Native container — no emulation |
| `arm64` | `ubuntu-latest` (x86-64) | **Cross-compiled** via `aarch64-linux-gnu-g++` — no QEMU |

### Build steps (inside Docker)

```
apt-get install cmake libcups2-dev libcupsimage2-dev libjbig-dev ...
cmake -S /workspace -B /tmp/splix-build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
cmake --build /tmp/splix-build --parallel $(nproc)
checkinstall ... cmake --install . --destdir /tmp/splix-install
find /tmp/splix-build -name "*.deb" -exec cp {} /workspace/artifacts/ \;
```

### Triggers

| Event | Action |
|---|---|
| Push to `main` / `master` | Builds both arches, uploads artifacts |
| Pull request to `main` | Builds both arches (validation only) |
| Push a `v*` tag | Builds both arches + attaches `.deb` to GitHub Release |
| `workflow_dispatch` | Manual trigger from the Actions UI |

---

## PPD File Map

The following table mirrors the `ppd/Makefile` brand-to-directory mapping
and is what is physically implemented in the `install(DIRECTORY ...)` rules
in `CMakeLists.txt`.

| CMake pattern | CUPS install directory | Printer brand |
|---|---|---|
| `clp*.ppd`, `clx*.ppd`, `m[0-9]*.ppd`, `ml*.ppd`, `scx*.ppd`, `sf*.ppd` | `$(CUPSPPD)/samsung/` | Samsung |
| `ph*.ppd`, `wc*.ppd` | `$(CUPSPPD)/xerox/` | Xerox |
| `1100*.ppd`, `1110*.ppd` | `$(CUPSPPD)/dell/` | Dell |
| `x215*.ppd` | `$(CUPSPPD)/lexmark/` | Lexmark |
| `es*.ppd` | `$(CUPSPPD)/toshiba/` | Toshiba |

> **HP (laser10x, laser13x):** The original `ppd/Makefile` lists HP models
> but no pre-built `.ppd` files exist in the repository for them.  They were
> generated from `hp.drv.in` at development time.  If HP support is needed,
> run `cups-ppdc ppd/hp.drv.in -d ppd/` and commit the resulting files.

---

## Troubleshooting

### `Could NOT find CUPS` / `Could not find a package configuration file provided by "CUPS"`

**Root cause:** `find_package(CUPS REQUIRED)` is the standard CMake call, but
CUPS does **not** ship a `CUPSConfig.cmake` or `cups-config.cmake` file.
CMake falls through to Config mode and fails with a misleading error message.

**This is why `CMakeLists.txt` uses `pkg_check_modules(CUPS REQUIRED cups)`
instead.** CUPS ships a standards-compliant `cups.pc` pkg-config file on
every Linux distribution. This is also what the original Makefile used.

If you still see this error after the fix, install the pkg-config package:

```bash
apt-get install pkg-config libcups2-dev
```

### `Could not find CUPSIMAGE_LIBRARY`

Install: `apt-get install libcupsimage2-dev`

### `Could not find JBIG_LIBRARY`

Install: `apt-get install libjbig-dev`

### `pstoqpdl.cpp: error: 'RASTERDIR' undeclared`

You are building with the old `make` (not CMake).  
With CMake, these are supplied via `target_compile_definitions`.  
With the old Makefile, verify `module.mk` still has `src_pstoqpdl_cpp_FLAGS`.

### `checkinstall: no .deb produced`

Check that you `cd` into the CMake build directory **before** running
`checkinstall`.  The `.deb` is written to the current working directory.

### ARM64 build is slow / how to speed it up

If you are building ARM64 locally without the cross-compiler, consider
installing `gcc-aarch64-linux-gnu` and using the toolchain file instead
of running inside a QEMU-emulated container.  See the
[Cross-Compilation](#cross-compilation-arm64) section above.

In CI, cross-compilation is already the default — no QEMU is used.

## Pre-Push Automated Testing

To prevent bad commits (such as failing CMake configuration, unlinked binaries, or broken macro definitions) from reaching the GitHub CI workflow, SpliX includes a localized native test simulator. 

### Running tests locally (Native Sandbox)
You can invoke the simulation manually at any time to verify that your current source tree successfully compiles with CMake and that the binaries link correctly natively:

```bash
./tests/test_build.sh
```

### Running tests locally (Docker CI Simulator)
To run the *exact* GitHub Actions pipeline logic (Debian oldstable compilation, dpkg-deb packaging, and artifact generation) locally through Docker, invoke the Docker CI simulator. 
It supports building for either `amd64` or `arm64` via the script arguments.

```bash
# Simulates CI pipeline for AMD64
./tests/test_ci_docker.sh amd64

# Simulates CI pipeline for ARM64 using multiarch cross-compiler
./tests/test_ci_docker.sh arm64
```
Once completed, it will drop the fully constructed `.deb` files into a local `./artifacts` directory.

### Enabling the Git pre-push hook (Recommended)
You can set git to automatically trigger the basic native sandbox tests and block your `git push` if they fail. To install the hook, simply run:

```bash
git config core.hooksPath .githooks
```
Now, whenever you fire a `git push`, the local `.githooks/pre-push` script will transparently launch the native `test_build.sh` sandbox to ensure all code compiles syntactically flawlessly before submitting to the CI workflow.
