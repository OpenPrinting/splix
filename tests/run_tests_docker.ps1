# SpliX Docker Test Runner (Windows PowerShell)
# Verifies: C++23, GTest Unit Tests, Functional Parity, and Package Generation

$ErrorActionPreference = "Stop"

# Get project root
$RootDir = Get-Location
$RootDirPath = $RootDir.Path.Replace('\', '/')

# Parameters
$Image = "splix-builder"
$CMakePreset = "ci-amd64"

Write-Host "--- SpliX Modernization: Docker Verification ---" -ForegroundColor Cyan

# Ensure artifact directory exists
if (-not (Test-Path "artifacts")) {
    New-Item -ItemType Directory -Path "artifacts" | Out-Null
}

$DockerCommand = @"
set -e
echo '>> Environment Ready (splix-builder)'
rm -rf build
cmake --preset $CMakePreset -B build
cmake --build build -j`$(nproc)
echo '>> Build Complete. Running Tests...'
cd build
ctest --output-on-failure
echo '>> Tests Passed. Generating Package...'
cpack -G DEB
mkdir -p /workspace/artifacts
cp *.deb /workspace/artifacts/
echo '>> Artifacts copied to /workspace/artifacts/'
"@

# Run Docker
docker run --rm `
  -v "${RootDirPath}:/workspace" `
  -w //workspace `
  -e DEBIAN_FRONTEND=noninteractive `
  $Image `
  bash -c $DockerCommand

Write-Host "--- Verification Complete ---" -ForegroundColor Green
