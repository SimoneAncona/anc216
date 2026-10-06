$ErrorActionPreference = "Stop"
Push-Location $PSScriptRoot
try {
    cmake -S . -B build -DANC216_WITH_SDL=ON @args
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    cmake --build build --parallel --config Release
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    ctest --test-dir build --build-config Release --output-on-failure
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
