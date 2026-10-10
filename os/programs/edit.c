// First C program using LLVM targetting ANC216
// utils pack version 0.2
#include "../libs/abi.h"

int main(void)
{
    kernel_service(SYS_PRINT, (unsigned long)"prova\n", 6, 0, 0);
    return 0;
}
