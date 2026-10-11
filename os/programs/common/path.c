#include "../../libs/abi.h"

unsigned get_full_path(char *path, unsigned pathlen, char *res)
{
    unsigned path_size = 0;
    unsigned i = 0;
    if (path[0] != '/')
    {
        path_size = kernel_service(SYS_GETCWD, (unsigned)res, OS_PATH_CAPACITY, 0, 0);
        if (get_err() != OS_SUCCESS)
            return 0;
        if (path_size > 1)
            res[path_size++] = '/';
    }
    if (path_size + pathlen >= OS_PATH_CAPACITY)
    {
        kernel_service(SYS_PRINT, (unsigned)"Path too long\n", 14, 0, 0);
        return 0;
    }
    unsigned argcnt = 0;
    for (; argcnt < pathlen; argcnt++)
    {
        res[argcnt + path_size] = path[argcnt];
    }
    path_size += pathlen;
    res[path_size] = 0;
    return path_size;
}
