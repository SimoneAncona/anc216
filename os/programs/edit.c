// First C program using LLVM targetting ANC216
// utils pack version 0.2
#include "../libs/abi.h"
#include "./common/print.c"
#include "./common/file.c"
#include "./common/path.c"

#define MEM (char *)0x7000
#define MAX_SIZE 0x1000
#define TITLE "edit          TAB to save & exit"

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
    header(TITLE, sizeof TITLE - 1);
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

int main(char *arg, unsigned arglen)
{
    if (arglen == 0)
    {
        kernel_service(SYS_PRINT, (unsigned)"A path is expected as arg\n", 26, 0, 0);
        return 1;
    }
    char os_path[OS_PATH_CAPACITY];
    unsigned i;
    unsigned path_size = get_full_path(arg, arglen, os_path);
    if (read_file(os_path, path_size, &i, MEM, MAX_SIZE) != OS_SUCCESS)
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
            if (save_file(os_path, path_size, i, MEM) != OS_SUCCESS)
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
