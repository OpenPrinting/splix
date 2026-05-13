#!/usr/bin/env bash
set -euo pipefail

TARGET_ARCH=${1:-amd64}

echo "=================================================="
echo " SPLIX CI/CD LOCAL DOCKER SIMULATOR"
echo " Architecture: $TARGET_ARCH"
echo "=================================================="
echo "This script runs the exact GitHub Actions logic"
echo "inside a local Docker container."
echo

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$ROOT_DIR"
mkdir -p artifacts
mkdir -p .ccache

if [ "$TARGET_ARCH" = "arm64" ]; then
    CMAKE_PRESET="ci-arm64"
else
    TARGET_ARCH="amd64"
    CMAKE_PRESET="ci-amd64"
fi

echo ">> Spawning debian:oldstable container... (This may take a minute)"

docker run --rm \
  -v "${ROOT_DIR}:/workspace" \
  -v "${ROOT_DIR}/.ccache:/root/.ccache" \
  -w /workspace \
  -e CCACHE_DIR=/root/.ccache \
  -e DEB_ARCH="${TARGET_ARCH}" \
  -e CMAKE_PRESET="${CMAKE_PRESET}" \
  -e GITHUB_REF_NAME="2.0.2" \
  splix-builder \
  bash -c '
    set -euo pipefail

    echo " [1/3] Configuring CMake..."
    # Ensure build dir is clean for a fresh cross-compile if needed
    rm -rf build/
    cmake --preset "${CMAKE_PRESET}"

    echo " [2/4] Compiling..."
    cmake --build --preset "${CMAKE_PRESET}"

    echo " [3/4] Running Unit Tests..."
    cd build/
    ctest --output-on-failure

    echo " [4/4] Generating Debian Package (.deb)..."
    cpack -G DEB

    # Move to artifacts
    mkdir -p /workspace/artifacts
    cp -v /workspace/build/*.deb /workspace/artifacts/

    echo "=================================================="
    echo " ✅ SUCCESS! Package generated."
    echo "=================================================="
    ls -lh /workspace/artifacts/
  '

echo "Check your local /artifacts directory for the generated .deb file!"
