#define SYS_EXIT  0x00                       // syscall exit ID = 0x00
#define SYS_OPEN  0x01                       // syscall open ID = 0x01
#define SYS_CLOSE  0x02                      // syscall close ID = 0x02
#define SYS_READ  0x03                       // syscall read ID = 0x03
#define SYS_WRITE  0x04                      // syscall write ID = 0x04
#define SYS_PRINT  0x05                      // syscall print ID = 0x05
#define SYS_GETL  0x06                       // syscall getl ID = 0x06
#define SYS_CLEAR  0x07                      // syscall clear ID = 0x07
#define SYS_MOUNT  0x10                      // syscall mount ID = 0x10
#define SYS_EXEC  0x11                       // syscall exec ID = 0x11
#define SYS_REWIND  0x12                     // syscall rewind ID = 0x12
#define SYS_SIZE  0x13                       // syscall size ID = 0x13
#define SYS_DIRECTORY  0x14                  // syscall directory ID = 0x14
#define SYS_LIST  0x15                       // syscall list ID = 0x15
#define SYS_TOUCH  0x16                      // syscall touch ID = 0x16
#define SYS_MKDIR  0x17                      // syscall mkdir ID = 0x17
#define SYS_REMOVE  0x18                     // syscall remove ID = 0x18
#define SYS_FSTAT  0x19                      // syscall fstat ID = 0x19
#define SYS_RUN  0x1a                        // syscall run ID = 0x1a
#define SYS_GETCWD  0x1b                     // syscall getcwd ID = 0x1b
#define SYS_CHDIR  0x1c                      // syscall chdir ID = 0x1c
#define SYS_POWEROFF  0x1d                   // syscall poweroff ID = 0x1d
#define SYS_VIDEO  0x1e                      // syscall video ID = 0x1e
#define SYS_GETK  0x1f                       // syscall getk ID = 0x1f
#define SYS_REDCT  0x20                      // syscall redct ID
#define PRINT_BYPASS_REDIRECT  0x0100        // R2 flag: print directly to the console
#define OS_STAT_BYTES  8                     // os stat bytes = 8
#define OS_SUCCESS  0                        // os success = 0
#define OS_INVALID  1                        // os invalid = 1
#define OS_ENTRY_BYTES  24                   // os entry bytes = 24
#define OS_PATH_CAPACITY  255                // os path capacity = 255
#define OS_ARGUMENT_POINTER  0x3000          // os argument pointer = 0x3000
#define OS_USER_BASE  0x4000                 // os user base = 0x4000
#define ASCII_NUL  0                         // ascii nul = 0
#define ASCII_SPACE  32                      // ascii space = 32
#define ASCII_SLASH  47                      // ascii slaxsh = 47
#define ASCII_DOT  46                        // ascii dot = 46
#define ASCII_NEWLINE  10                    // ascii newline = 10
#define OS_ENTRY_DIRECTORY  1                // os entry directory = 1
#define ASCII_DIGIT_ZERO  48                 // ascii digit zero = 48
#define ASCII_DIGIT_NINE  57                 // ascii digit nine = 57
#define DECIMAL_MULTIPLY_LIMIT  6553         // decimal multiply limit = 6553
#define OS_NOT_FOUND  3                      // os not found = 3
#define OS_CAPACITY  6                       // os capacity = 6
#define OS_BAD_EXEC  7                       // os bad exec = 7
#define OS_UNSUPPORTED  8                    // os unsupported = 8
#define OS_PERMISSION  9                     // os permission = 9
#define OS_EXISTS  10                        // os exists = 10
#define OS_NOT_EMPTY  11                     // os not empty = 11

extern unsigned kernel_service(unsigned service, unsigned arg0, unsigned arg1, unsigned arg2, unsigned arg3);
extern unsigned get_err(void);  // used to get eventual kernel_service error codes
