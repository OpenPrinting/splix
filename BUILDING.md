# Building SpliX 3.0.0

SpliX 3.0.0 uses CMake and requires a compiler/standard-library pair with the
C++23 facilities used by the driver: `std::expected`, `std::span`,
`std::semaphore`, and `std::jthread`.

## Native build

On Debian 13 (trixie) or a compatible Ubuntu release:

```bash
sudo apt-get update
sudo apt-get install \
  build-essential cmake pkg-config cups \
  libcups2-dev libcupsimage2-dev libjbig-dev

cmake --preset linux-amd64-release
cmake --build --preset linux-amd64-release --parallel
ctest --test-dir build-amd64 --output-on-failure
```

The minimum supported CMake version is 3.25. Configuration performs an actual
C++23 library probe; compiler version numbers alone are not treated as proof of
support.

## Debug and sanitizer builds

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug --parallel
ctest --test-dir build-debug --output-on-failure

cmake --preset linux-asan
cmake --build --preset linux-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

## Cross-compilation

CI cross-compiles Debian packages for `arm64`, `armhf`, `i386`, and `riscv64`.
For example, an ARM64 build on Debian requires:

```bash
sudo dpkg --add-architecture arm64
sudo apt-get update
sudo apt-get install \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
  libcups2-dev:arm64 libcupsimage2-dev:arm64 libjbig-dev:arm64

cmake --preset linux-arm64-release
cmake --build --preset linux-arm64-release --parallel
```

The `ci-*` presets use `/tmp/splix-build` and are intended for the GitHub
Actions container. Toolchain files under `cmake/` select the target compiler,
library roots, and target `pkg-config` directory.

## Package generation

After a successful build:

```bash
cd build-amd64
cpack -G DEB
```

The resulting name follows `splix-3.0.0-<architecture>.deb`.

## Install paths and options

Common overrides:

```bash
cmake -S . -B build \
  -DSPLIX_FILTER_DIR=/usr/lib/cups/filter \
  -DSPLIX_PPD_DIR=/usr/share/cups/model \
  -DSPLIX_PROFILE_DIR=/usr/share/cups/profiles
```

Feature options:

- `DISABLE_JBIG=ON` disables algorithms that require libjbig.
- `DISABLE_THREADS=ON` disables threaded compression.
- `DISABLE_BLACKOPTIM=ON` disables CMYK black optimization.
- `THREADS=<n>` selects compression worker count.
- `CACHESIZE=<pages>` selects the maximum resident compressed-page count.

Install to the system or a staging root with:

```bash
sudo cmake --install build-amd64
DESTDIR=/tmp/splix-stage cmake --install build-amd64
```

## Verification

The native test suite includes byte-level compression characterization,
concurrency behavior, filter functional tests, ASan/UBSan execution, and
multi-architecture compile/package checks.

Run the Docker-oriented helper used by contributors with:

```bash
./tests/run_tests_docker.sh
```

Generated PPD files are committed and installed directly; end users do not
need `cups-ppdc` to build the driver.

## Troubleshooting

- `SpliX requires C++23 library support for std::expected`: use a newer
  compiler and standard library together. A new compiler with an older library
  can still fail this probe.
- `CUPS development headers not found`: install `libcups2-dev` and
  `pkg-config`.
- `libcupsimage not found`: install `libcupsimage2-dev`.
- `libjbig not found`: install `libjbig-dev` or configure with
  `-DDISABLE_JBIG=ON`.
