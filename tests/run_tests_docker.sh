#!/usr/bin/env bash
# SpliX Docker Test Runner
# Verifies: C++23, GTest Unit Tests, Functional Parity, and Package Generation

set -euo pipefail

TARGET_ARCH=${1:-amd64}
CMAKE_PRESET="ci-${TARGET_ARCH}"
IMAGE="debian:bookworm"

echo "=================================================="
echo " 🛠️  SPLIX DOCKER TEST RUNNER"
echo " Architecture: $TARGET_ARCH"
echo " Target Image: $IMAGE"
echo "=================================================="

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$ROOT_DIR"
mkdir -p artifacts
mkdir -p .ccache

echo ">> Launching container..."

# Disable path conversion for Git Bash to prevent -w /workspace becoming a local path
export MSYS_NO_PATHCONV=1

docker run --rm \
  -v "${ROOT_DIR}:/workspace" \
  -v "${ROOT_DIR}/.ccache:/root/.ccache" \
  -w //workspace \
  -e CCACHE_DIR=/root/.ccache \
  -e CMAKE_PRESET="${CMAKE_PRESET}" \
  -e DEBIAN_FRONTEND=noninteractive \
  "${IMAGE}" \
  bash -c '
    set -euo pipefail

    echo " [1/6] Installing dependencies..."
    apt-get update -y
    apt-get install -y --no-install-recommends \
      build-essential cmake pkg-config ccache \
      libcups2-dev libcupsimage2-dev libjbig-dev \
      ghostscript gettext \
      curl git ca-certificates

    echo ">> Diagnostic: algo0x11.cpp first 40 lines"
    cat /workspace/src/algo0x11.cpp | head -n 40
    echo ">> Diagnostic: document.cpp lines 90-100"
    cat /workspace/src/document.cpp | sed -n '90,100p'

    echo ">> Dependency versions:"
    cmake --version | head -n 1
    gcc --version | head -n 1
    pkg-config --version

    export PATH="/usr/lib/ccache:${PATH}"

    echo " [2/6] Verifying C++23 Support..."
    chmod +x scripts/check_cpp23.sh
    ./scripts/check_cpp23.sh

    echo " [3/6] Configuring CMake (Preset: ${CMAKE_PRESET})..."
    rm -rf /tmp/splix-build
    cmake --preset "${CMAKE_PRESET}" -B /tmp/splix-build

    echo " [4/6] Building SpliX..."
    cmake --build /tmp/splix-build -j$(nproc)

    echo " [5/6] Running Tests (CTest + GTest)..."
    cd /tmp/splix-build
    ctest --output-on-failure --test-action Test

    echo " [6/6] Generating Debian Package..."
    cpack -G DEB > /dev/null
    cp -v /tmp/splix-build/*.deb /workspace/artifacts/

    echo "=================================================="
    echo " ✅ ALL TESTS PASSED!"
    echo "=================================================="
    ls -lh /workspace/artifacts/
  '
