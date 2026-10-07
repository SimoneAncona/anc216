$ErrorActionPreference = "Stop"
Push-Location $PSScriptRoot
try {
    cmake -S . -B build/production -DANC216_WITH_SDL=ON @args -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    cmake --build build/production --parallel --config Release
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    cmake --build build/production --target os --parallel --config Release
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
