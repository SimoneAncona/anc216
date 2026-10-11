#include "../libs/abi.h"
#include "./common/print.c"

int main(char *arg, unsigned arglen)
{
    if (arglen == 0)
    {
        kernel_service(SYS_PRINT, (unsigned)"A path is expected as arg\n", 26, 0, 0);
        return 1;
    }
    return 0;
}
