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
    cmake ../ --config Release
} else {
    if ($args.Contains("-t=linux")) {
        & wsl -e sh ./build.sh
        exit
    }
    Set-Location build
    cmake ../
}
$vs = (Get-CimInstance MSFT_VSInstance -Namespace root/cimv2/vs).InstallLocation
$msbuild = "MSBuild\Current\Bin\MSBuild.exe"
& "$vs\$msbuild" ((Get-Item *.sln).name)
Set-Location ../