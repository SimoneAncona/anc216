if ($args[0] -eq "--release") {
    Set-Location src; & ..\build\Release\*.exe $args[1] $args[2]; cd..
} else {
    Set-Location src; & ..\build\Debug\*.exe $args[0] $args[1]; cd..
}