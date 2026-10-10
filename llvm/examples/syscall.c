#include <anc216/syscall.h>

/* SYS_CLEAR: requires an OS user process with console permission.
 * The compiler's user-mode stack-local lowering is not implemented yet.
 */
int main(void)
{
    unsigned args[6];
    args[0] = args[1] = args[2] = args[3] = args[4] = args[5] = 0;
    return syscall(0x07, args);
}
