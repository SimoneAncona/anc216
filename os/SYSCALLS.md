# User syscall ABI

Load the service number into L0, arguments into registers, then execute `syscall`. R0–R5 are preserved, R6 returns a count/value, and R7 returns zero on success or an error. Pointers are logical user IMEM offsets, never host pointers. Buffer sizes are bytes. Buffers must stay below logical offset `3ff0`, which reserves the mode-switch stub.

Services 00–06 use the original PDF's numbering. The fwrite packing below resolves its overlapping mode/size description. 07 and 10–1c are OS extensions (hexadecimal).

| L0 | Service | Arguments | Result |
| --- | --- | --- | --- |
| 00 | exit | R1 exit code | Terminates process; kernel idle |
| 01 | fopen | R1 absolute path, L2 length, R3 descriptor-byte pointer | Writes descriptor 1, R6=1 |
| 02 | fclose | R1 descriptor 1 | Closes file |
| 03 | fread | R1 descriptor, L2 count, R3 destination | R6 bytes read, zero at EOF |
| 04 | fwrite | R1 descriptor, R2=`8000 OR count`, R3 source | R6 overwritten bytes; no growth |
| 05 | print | R1 text, L2 count, R3 stream 0/1 | R6 printed bytes |
| 06 | getl | R1 capacity 1–255 including NUL, R2 destination | R6 length excluding newline/NUL |
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
| 1a | run | R1 executable path, L2 length, R3 argument bytes, R4 argument length <255 | Replace process; exit/fault reloads init |
| 1b | getcwd | R1 destination, R2 capacity including NUL | R6 cwd length excluding NUL |
| 1c | chdir | R1 normalized absolute directory path, L2 length | Change session cwd |
| 1d | poweroff | none | Poweroff the system |

Print/getl/clear require UALf flag 80. Filesystem services require flag 20. Exit is always allowed. Missing services, including listenkey, sleep and malloc, return error 8. Streams 0/1 currently use the same console.

Getl waits for Enter, NUL-terminates the buffer and omits the newline from its result. Backspace removes a buffered byte and erases its glyph, restoring the previous cell across row wraps. An underline cursor blinks every 500 ms while waiting; it is removed on Enter. Characters beyond capacity are ignored until Enter. An absent keyboard returns error 2.

| R7 | Meaning |
| --- | --- |
| 1 | Invalid argument or buffer |
| 2 | Absent/wrong device or invalid AFS magic/version |
| 3 | Path not found |
| 4 | Invalid/closed descriptor |
| 5 | Corrupt selected file chain |
| 6 | No free file-head cluster or directory slot |
| 7 | Invalid, unsupported, or oversized UALf |
| 8 | Unsupported syscall |
| 9 | UALf permission denied |
| 10 | A file/directory already occupies the requested name |
| 11 | Directory is not empty |

A successful short read/write is not an error; inspect R6. Mount selects any MPME address, allowing programs to access many independent 64 KiB volumes. Only one file descriptor is currently supported; opening replaces it. The emulator does not persist guest chip writes to host files.

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
