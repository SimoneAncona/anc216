// First C program using LLVM targetting ANC216
// utils pack version 0.2
#include "../libs/abi.h"

#define AVC64ADR 0xfffd
#define DEBUG kernel_service(SYS_PRINT, (unsigned)"DEBUG", 5, 0, 0)
#define MEM (char *)0x7000
#define MAX_SIZE 0x1000

void print_char(unsigned char x, unsigned char y, unsigned char fg, unsigned char bg, unsigned char c)
{
    wemem(AVC64ADR, 0x0300 + x);
    wemem(AVC64ADR, 0x0400 + y);
    wemem(AVC64ADR, 0x0600 + c);
    wemem(AVC64ADR, 0x0700 + fg); // set D3 (fg color) to black (00)
    wemem(AVC64ADR, 0x0800 + bg); // set D4 (bg color) to white (FF)
    wemem(AVC64ADR, 0x0100);
}

void header(void)
{
    // using the AVC64 video card in texture mode
    // In D1 and D2 you store the texture ID (in this case ascii char code)
    // D3 for foreground color
    // D4 for background color
    // CX for X pos
    // CY pos Y pos
    // AOS0.2 supports 8x8 ascii charmap, so coordinates will be
    // Y=0, X=0 for first char
    // Y=0, X=8 for the next one, and so on
    const char *header_st = "edit   TAB to save & exit";
    wemem(AVC64ADR, 0x0700); // set D3 (fg color) to black (00)
    wemem(AVC64ADR, 0x08FF); // set D4 (bg color) to white (FF)
    wemem(AVC64ADR, 0x0400); // set CY = 0
    wemem(AVC64ADR, 0x0500); // ascii uses only 1 byte, so D1 is always 0
    unsigned i = 0;
    for (; i < 25; i++)
    {
        wemem(AVC64ADR, 0x0300 + (i << 3));     // set CX = i * 8
        wemem(AVC64ADR, 0x0600 + header_st[i]); // set D2 = ascci char code
        wemem(AVC64ADR, 0x0100);                // print
    }
    wemem(AVC64ADR, 0x0620); // set D2 = space char
    for (; i < 32; i++)
    {
        wemem(AVC64ADR, 0x0300 + (i << 3)); // set CX = i * 8
        wemem(AVC64ADR, 0x0100);
    }
    // reset color schema
    wemem(AVC64ADR, 0x07FF); // set D3 (fg color) to white (FF)
    wemem(AVC64ADR, 0x0800); // set D4 (bg color) to black (00)
    // print a new line
    kernel_service(SYS_PRINT, (unsigned)"\n", 1, 0, 0);
}

void print_line(unsigned *cursor, unsigned start_col, unsigned line, unsigned buff_size)
{
    char *buff = MEM;
    while (start_col != 0 && *cursor < buff_size)
    {
        if (buff[*cursor] == '\n')
        {
            (*cursor)++;
            return;
        }
        (*cursor)++;
        start_col--;
    }
    int i = 0;
    unsigned char print = 1;
    while (*cursor < buff_size && buff[*cursor] != '\n')
    {
        if (print)
            print_char(i << 3, line << 3, 0xff, 0x00, buff[*cursor]);
        (*cursor)++;
        i++;
        if (i > 31)
        {
            print = 0;
        }
    }
    if (*cursor >= buff_size)
        print_char(i << 3, line << 3, 0xff, 0x00, '|');
    (*cursor)++;
}

void print_current_window(unsigned cursor)
{
    kernel_service(SYS_CLEAR, 0, 0, 0, 0); // clear the screen
    header();
    char *buff = MEM;
    unsigned line = 0;
    unsigned column = 0;
    unsigned i = 0;
    for (; i < cursor; i++)
    {
        if (buff[i] == '\n')
        {
            column = 0;
            line++;
            continue;
        }
        column++;
    }
    if (line > 27)
        line -= 27;
    else
        line = 0;
    if (column > 31)
        column -= 31;
    else
        column = 0;

    i = 0;
    while (line != 0 && i < cursor)
    {
        if (buff[i] == '\n')
            line--;
        i++;
    }
    int j = 0;
    for (; j < 27 && i < cursor; j++)
        print_line(&i, column, j + 1, cursor);
}

unsigned save_file(char *path, unsigned pathlen, unsigned size)
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
        written = kernel_service(SYS_WRITE, fd, 0x8000 | count, (unsigned)(MEM + offset), 0);
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

unsigned read_file(char *path, unsigned pathlen, unsigned *size)
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
    if (error != OS_SUCCESS || total > MAX_SIZE)
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
        received = kernel_service(SYS_READ, fd, count, (unsigned)(MEM + *size), 0);
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

int main(char *arg, unsigned arglen)
{
    if (arglen == 0)
    {
        kernel_service(SYS_PRINT, (unsigned)"A path is expected as arg\n", 26, 0, 0);
        return 1;
    }
    char os_path[OS_PATH_CAPACITY];
    unsigned path_size = 0;
    unsigned i = 0;
    if (arg[0] != '/')
    {
        path_size = kernel_service(SYS_GETCWD, (unsigned)os_path, OS_PATH_CAPACITY, 0, 0);
        if (get_err() != OS_SUCCESS)
            return 1;
        if (path_size > 1)
            os_path[path_size++] = '/';
    }
    if (path_size + arglen >= OS_PATH_CAPACITY)
    {
        kernel_service(SYS_PRINT, (unsigned)"Path too long\n", 14, 0, 0);
        return 1;
    }
    unsigned argcnt = 0;
    for (; argcnt < arglen; argcnt++)
    {
        os_path[argcnt + path_size] = arg[argcnt];
    }
    path_size += arglen;
    os_path[path_size] = 0;
    if (read_file(os_path, path_size, &i) != OS_SUCCESS)
    {
        kernel_service(SYS_PRINT, (unsigned)"Cannot read file\n", 17, 0, 0);
        return 1;
    }
    kernel_service(SYS_CLEAR, 0, 0, 0, 0); // clear the screen
    kernel_service(SYS_VIDEO, 1, 0, 0, 0); // clear the screen
    char *buff = MEM;                      // pointer to a custom memory region used as heap
    // Start editing at the end of the loaded file.
    print_current_window(i);
    while (1)
    {
        char k = kernel_service(SYS_GETK, 0, 0, 0, 0);
        if (k == '\b')
        {
            if (i == 0)
                continue;
            i--;
            buff[i] = 0;
            print_current_window(i);
        }
        else if (k == '\t')
        {
            if (save_file(os_path, path_size, i) != OS_SUCCESS)
            {
                kernel_service(SYS_PRINT, (unsigned)"Save failed; TAB to retry",
                               PRINT_BYPASS_REDIRECT | 24, 0, 0);
                continue;
            }
            break;
        }
        else
        {
            if (i >= MAX_SIZE)
                continue;
            buff[i] = k;
            i++;
            print_current_window(i);
        }
    }
    kernel_service(SYS_CLEAR, 0, 0, 0, 0); // clear the screen
    return 0;
}
