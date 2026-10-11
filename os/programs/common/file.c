#include "../../libs/abi.h"

unsigned read_file(char *path, unsigned pathlen, unsigned *size, char *buf, const int max_size)
{
    unsigned char fd;
    unsigned error;
    unsigned total;
    *size = 0;
    kernel_service(SYS_TOUCH, (unsigned)path, pathlen, 0, 0);
    error = get_err();
    if (error != OS_SUCCESS)
        return error;
    kernel_service(SYS_OPEN, (unsigned)path, pathlen, (unsigned)&fd, 0);
    error = get_err();
    if (error != OS_SUCCESS)
        return error;
    total = kernel_service(SYS_SIZE, fd, 0, 0, 0);
    error = get_err();
    if (error != OS_SUCCESS || total > max_size)
    {
        kernel_service(SYS_CLOSE, fd, 0, 0, 0);
        return error != OS_SUCCESS ? error : OS_CAPACITY;
    }
    while (*size < total)
    {
        unsigned count = total - *size;
        unsigned received;
        if (count > 255)
            count = 255;
        received = kernel_service(SYS_READ, fd, count, (unsigned)(buf + *size), 0);
        error = get_err();
        if (error != OS_SUCCESS || received != count)
        {
            kernel_service(SYS_CLOSE, fd, 0, 0, 0);
            return error != OS_SUCCESS ? error : OS_INVALID;
        }
        *size += received;
    }
    kernel_service(SYS_CLOSE, fd, 0, 0, 0);
    return get_err();
}

unsigned save_file(char *path, unsigned pathlen, unsigned size, char *buf)
{
    unsigned char fd;
    unsigned offset = 0;
    unsigned error;
    kernel_service(SYS_OPEN, (unsigned)path, pathlen, (unsigned)&fd, 0);
    error = get_err();
    if (error != OS_SUCCESS)
        return error;
    while (offset < size)
    {
        unsigned count = size - offset;
        unsigned written;
        if (count > 255)
            count = 255;
        written = kernel_service(SYS_WRITE, fd, 0x8000 | count, (unsigned)(buf + offset), 0);
        error = get_err();
        if (error != OS_SUCCESS || written != count)
        {
            kernel_service(SYS_CLOSE, fd, 0, 0, 0);
            return error != OS_SUCCESS ? error : OS_INVALID;
        }
        offset += written;
    }
    kernel_service(SYS_CLOSE, fd, 0, 0, 0);
    return get_err();
}
