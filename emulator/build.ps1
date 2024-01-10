if ($args[0] -eq "--help") {
    Write-Output "Usage:
    build
Options:
    --debug     Build for debug (default)
    --help      Show this help message
    --release   Build for release
    -t=linux    Build for linux
    -t=win32    Build for Windows
"
    exit
}
if (-not (Test-Path build/)) {
    mkdir build
}
if ($args.Contains("--release")) {
    if ($args.Contains("-t=linux")) {
        & wsl -e sh ./build.sh
        exit
    }
    Set-Location build
    cmake -DCMAKE_BUILD_TYPE=Release ../
    cmake --build . --config Release
} else {
    if ($args.Contains("-t=linux")) {
        & wsl -e sh ./build.sh
        exit
    }
    Set-Location build
    cmake  -DCMAKE_BUILD_TYPE=Debug ../
    cmake --build . --config Debug
}
Set-Location ../