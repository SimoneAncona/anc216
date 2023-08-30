if (-not (Test-Path build/)) {
    mkdir build
}
Set-Location build
cmake ../
$vs = (Get-CimInstance MSFT_VSInstance).InstallLocation[0]
$msbuild = "MSBuild\Current\Bin\MSBuild.exe"
& "$vs\$msbuild" assembler.sln
Set-Location ../