#!/usr/bin/env bash
set -euo pipefail

echo "=================================================="
echo " SPLIX BUILD-AND-TEST QA RUNNER"
echo "=================================================="
echo "This script simulates the CI workflow locally to"
echo "catch syntax, CMake, and linking errors early."
echo

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$ROOT_DIR"

# Ensure we're cleaning any previous test artifacts
rm -rf build-test
mkdir build-test

echo "1) Checking CMake configuration..."
if ! cmake -S . -B build-test -DCMAKE_BUILD_TYPE=Debug 2>&1 | tee build-test/cmake_configure.log; then
    echo "❌ CMake configuration failed."
    exit 1
fi
echo "✅ CMake configured successfully."

echo "2) Building core binaries..."
if ! cmake --build build-test --parallel 2>&1 | tee build-test/cmake_build.log | tail -n 5; then
    echo "❌ CMake build failed. Full log: build-test/cmake_build.log"
    tail -n 30 build-test/cmake_build.log
    exit 1
fi
echo "✅ CMake build completed successfully."

echo "3) Executing binary health checks..."
# The filters output their usage statement to stderr and exit with code 1.
# We intercept the execution and check if the expected usage signature exists.
#
# For cross-compiled binaries (e.g. ARM64 on x86-64), we skip execution tests
# and only verify the binary was produced.

verify_binary() {
    local bin_name=$1
    if [ ! -f "build-test/$bin_name" ]; then
        echo "❌ Binary $bin_name was not generated!"
        exit 1
    fi

    # Check if the binary is for the host architecture
    local file_output
    file_output=$(file "build-test/$bin_name" 2>/dev/null || true)
    local host_arch
    host_arch=$(uname -m)

    # If the binary doesn't match the host, skip execution test
    if [[ "$host_arch" == "x86_64" ]] && [[ "$file_output" != *"x86-64"* && "$file_output" != *"x86_64"* ]]; then
        echo "⏭️  $bin_name is cross-compiled (not x86-64). Skipping execution test."
        return 0
    fi
    if [[ "$host_arch" == "aarch64" ]] && [[ "$file_output" != *"aarch64"* && "$file_output" != *"ARM"* ]]; then
        echo "⏭️  $bin_name is cross-compiled (not aarch64). Skipping execution test."
        return 0
    fi

    # Run the binary. By design, it returns 1 and prints "Usage: " to stderr.
    set +e
    OUTPUT=$(./"build-test/$bin_name" 2>&1)
    EXIT_CODE=$?
    set -e

    if [[ "$EXIT_CODE" -ne 1 ]]; then
        echo "❌ $bin_name exited with unexpected code $EXIT_CODE."
        exit 1
    fi

    if [[ "$OUTPUT" != *"Usage:"* ]]; then
        echo "❌ $bin_name failed syntax usage check. Output was:"
        echo "$OUTPUT"
        exit 1
    fi
    echo "✅ $bin_name binary execution health check passed."
}

verify_binary "rastertoqpdl"
verify_binary "pstoqpdl"

echo
echo "=================================================="
echo " ✅ ALL LOCAL PRE-PUSH CHECKS PASSED!"
echo "=================================================="
