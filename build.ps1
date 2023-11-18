if (Test-Path .\assembler\build\) {
    Remove-Item -r assembler\build 
}
if (Test-Path .\cardreader\build\) {
    Remove-Item -r cardreader\build 
}
Set-Location .\assembler\; .\build.ps1 $args; cd..
Set-Location .\cardreader\; .\build.ps1 $args; cd..