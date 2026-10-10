# User syscall ABI

Load the service number into L0, arguments into registers, then execute `syscall`. R0–R5 are preserved, R6 returns a count/value, and R7 returns zero on success or an error. Pointers are logical user IMEM offsets, never host pointers. Buffer sizes are bytes. Buffers must stay below logical offset `3ff0`, which reserves the mode-switch stub.

User PC/SP/BP are also logical offsets. The initial SP/BP is `3800`, mapping to
physical `7800` with IMEM base `4000`. Interrupts save those logical values before
selecting the physical kernel stack; syscall return restores the same user context.

Services 00–06 use the original PDF's numbering. The fwrite packing below resolves its overlapping mode/size description. 07 and 10–1e are OS extensions (hexadecimal).

| L0 | Service | Arguments | Result |
| --- | --- | --- | --- |
| 00 | exit | R1 exit code | Terminates process; kernel idle |
| 01 | fopen | R1 absolute path, L2 length, R3 descriptor-byte pointer | Writes descriptor 1, R6=1 |
| 02 | fclose | R1 descriptor 1 | Closes file |
| 03 | fread | R1 descriptor, L2 count, R3 destination | R6 bytes read, zero at EOF |
| 04 | fwrite | R1 descriptor, R2=`8000 OR count`, R3 source | R6 bytes written; grows the file at EOF |
| 05 | print | R1 text, L2 count, R2 bit 8 bypasses redirection | R6 printed bytes |
| 06 | getl | R1 capacity 1–255 including NUL, R2 destination | R6 length excluding newline/NUL, in R2 the string captured from input |
| 07 | clear | No arguments | Clear display and reset text position |
| 10 | mount | R1 MPME EMEM address | Selects AFS volume; closes open file |
| 11 | exec | R1 absolute path, L2 length | Replaces process; errors return |
| 12 | rewind | R1 descriptor | Resets file position |
| 13 | size | R1 descriptor | R6 entire file size |
| 14 | directory | R1 absolute path, L2 length | R6 directory ID; root=0 |
| 15 | list | R1 directory path, L2 length, R3 24-byte output, R4 cursor | R6 next cursor, zero at EOF |
| 16 | touch | R1 absolute path, L2 length | Create empty file or keep existing file |
| 17 | mkdir | R1 absolute path, L2 length | Create one directory |
| 18 | remove | R1 absolute path, L2 length | Remove file chain or empty directory |
| 19 | fstat | R1 absolute path, L2 length, R3 8-byte output | R6 payload bytes; record below |
| 1a | run | R1 executable path, L2 length, R3 argument bytes, R4 argument length <255 | Replace process; exit/fault reloads /bin/init |
| 1b | getcwd | R1 destination, R2 capacity including NUL | R6 cwd length excluding NUL |
| 1c | chdir | R1 normalized absolute directory path, L2 length | Change session cwd |
| 1d | poweroff | none | Poweroff the system |
| 1e | video | R1=1 grant, R1=0 revoke | R6=`fffd` on grant (which is the standard AVC64 address in EMEM), zero on revoke |
| 1f | getk | none | R6=key code |
| 20 | redct | R1 absolute path (null to stop redirect), L2 length | Redirect all console stream to the specified file |

Print/getl/clear/video require UALf flag 80. Filesystem services, including `redct`, require flag 20. Once a redirect target is authorized, console programs can print to it without filesystem permission. Exit and poweroff are always allowed. Missing services, including listenkey, sleep and malloc, return error 8. `print` normally follows the active redirection. Set R2 to `PRINT_BYPASS_REDIRECT OR count` (`0100 OR count`) to send just that call to the console, leaving redirection and its file cursor intact. The shell uses this flag for both the cwd and `> ` prompt; `app_print_console` is the user helper. Redirection persists across commands: run `redct` without arguments to stop it before displaying a file with `cat`.

Getl waits for Enter, NUL-terminates the buffer and omits the newline from its result. Backspace removes a buffered byte and erases its glyph, restoring the previous cell across row wraps. An underline cursor blinks every 500 ms while waiting; it is removed on Enter. Characters beyond capacity are ignored until Enter. An absent keyboard returns error 2.

| R7 | Meaning |
| --- | --- |
| 1 | Invalid argument or buffer |
| 2 | Absent/wrong device or invalid AFS magic/version |
| 3 | Path not found |
| 4 | Invalid/closed descriptor |
| 5 | Corrupt selected file chain |
| 6 | No free cluster or directory slot |
| 7 | Invalid, unsupported, or oversized UALf |
| 8 | Unsupported syscall |
| 9 | UALf permission denied |
| 10 | A file/directory already occupies the requested name |
| 11 | Directory is not empty |

Reads stop successfully at EOF. Writes overwrite from the current cursor and grow the file at EOF, allocating continuation clusters as needed. A full volume returns error 6; R6 reports bytes written before capacity was exhausted. A zero-byte write does not allocate storage. Mount selects any MPME address, allowing programs to access many independent 64 KiB volumes. Only one file descriptor is currently supported; opening replaces it. MPME writes update and flush the attached host file immediately. Rebuilding generated cards replaces their contents; use separate copies for personal data.

```asm
load r0, 5
load r1, message
load r2, sizeof message
load r3, 0
syscall
cmp r7, 0
jne error
```

The kernel uses the CPU's interrupt frame and saved SP/BP slots to return, rather than RET. All kernel helpers document their register contracts; user programs must keep three bytes above SP available for syscall return.

## Directory records and command entry

`list` uses cursor 0 to start. Pass its nonzero R6 back as R4 to resume. A cursor
scans directory IDs 1..96 then file heads 1..196 (combined cursor 97..292),
skipping free slots and continuation clusters. At EOF, R6=0 and output kind=0.
A 24-byte record contains kind at byte 0 (1 directory, 2 file), followed by a
NUL-terminated name, including `.` before a file extension when present. Directory
listing/lookup and namespace mutations close the current file descriptor.

`touch` is idempotent for an existing validated file and does not truncate it.
`mkdir` requires an existing parent and reports error 10 on a name collision.
`remove` validates a file's complete chain before freeing any allocation. Root
cannot be removed; nonempty directories return 11. Writes still cannot extend
an empty file created with touch. Paths must be absolute and nonempty, with no
embedded NUL; the user path library supplies canonical paths for shell commands.

`run` copies arguments before clearing the old user's memory. At command entry,
R1=`0x3000` is the logical argument pointer and R2 its byte count; a NUL follows
the bytes. This range is below the stack. Commands are loaded from the boot chip;
the session's selected data volume is restored before execution. Successful run
never returns to the old shell; exit/fault reloads init. Failed launches return
the ordinary error and preserve the caller. Plain `exec` retains its original
replacement semantics and does not request a return to init.

## File and directory disk usage

`SYS_FSTAT` requires filesystem permission and closes the current descriptor.
Its output is four big-endian 16-bit words:

| Offset | Meaning |
| --- | --- |
| 0 | Kind: 1 directory, 2 file |
| 2 | Payload size in bytes |
| 4 | Allocated disk bytes |
| 6 | Allocation units: file clusters plus directory records |

A file uses complete 323-byte clusters, including metadata and slack. A directory
includes itself and all descendants: each non-root directory owns a 16-byte
record; root owns none. Payload is the sum of descendant file sizes. Global boot,
allocation-table and reserved overhead is excluded. Selected chains and parent
walks are validated; corrupt chains or overflowing totals return error 5. For
example, a 650-byte file takes three clusters: 969 allocated bytes. Its otherwise
empty parent directory reports 650 payload bytes, 985 allocated bytes and 4 units.
`stat PATH` displays these values.

## Direct video access

A fresh process starts with an empty EMEM grant (lower=`ffff`, upper=`0000`).
`SYS_VIDEO(1)` checks console permission and the AVC64 identity, then sets both
bounds to physical device address `fffd`. User READ/WRITE can now access the AVC64
command protocol directly. Addresses are absolute, without EMEM rebasing.
`SYS_VIDEO(0)` revokes the grant; successful exec/run and return to init also reset
it. Revocation does not clear the display. Other arguments return error 1; an
absent/wrong display returns 2. Keyboard `fffc` remains outside the grant.
IREQ/REQ/HREQ/HWRITE/PAREQ/CAREQ and MTU setters remain privileged. The grant is
whole-device access, including shared console drawing state.
