if ($args[0] -eq "--release") {
    Set-Location src; & ..\build\Release\assembler.exe $args[1] $args[2] $args[3] $args[4]; cd..
} else {
    Set-Location src; & ..\build\Debug\assembler.exe $args[0] $args[1] $args[2] $args[3]; cd..
}