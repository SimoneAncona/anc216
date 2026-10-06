# User syscall ABI

Load the service number into L0, arguments into registers, then execute `syscall`. R0–R5 are preserved, R6 returns a count/value, and R7 returns zero on success or an error. Pointers are logical user IMEM offsets, never host pointers. Buffer sizes are bytes. Buffers must stay below logical offset `3ff0`, which reserves the mode-switch stub.

Services 00–06 use the original PDF's numbering. The fwrite packing below resolves its overlapping mode/size description. 10–13 are OS extensions (hexadecimal).

| L0 | Service | Arguments | Result |
| --- | --- | --- | --- |
| 00 | exit | R1 exit code | Terminates process; kernel idle |
| 01 | fopen | R1 absolute path, L2 length, R3 descriptor-byte pointer | Writes descriptor 1, R6=1 |
| 02 | fclose | R1 descriptor 1 | Closes file |
| 03 | fread | R1 descriptor, L2 count, R3 destination | R6 bytes read, zero at EOF |
| 04 | fwrite | R1 descriptor, R2=`8000 OR count`, R3 source | R6 overwritten bytes; no growth |
| 05 | print | R1 text, L2 count, R3 stream 0/1 | R6 printed bytes |
| 06 | getl | R1 capacity 1–255 including NUL, R2 destination | R6 length excluding newline/NUL |
| 10 | mount | R1 MPME EMEM address | Selects AFS volume; closes open file |
| 11 | exec | R1 absolute path, L2 length | Replaces process; errors return |
| 12 | rewind | R1 descriptor | Resets file position |
| 13 | size | R1 descriptor | R6 entire file size |

Print/getl require UALf flag 80. Filesystem services require flag 20. Exit is always allowed. Missing services, including listenkey, sleep, malloc, mkdir and deletion, return error 8. Streams 0/1 currently use the same console.

Getl waits for Enter, NUL-terminates the buffer and omits the newline from its result. Backspace removes a buffered byte; visual backspace erasure is not implemented. Characters beyond capacity are ignored until Enter. An absent keyboard returns error 2.

| R7 | Meaning |
| --- | --- |
| 1 | Invalid argument or buffer |
| 2 | Absent/wrong device or invalid AFS magic/version |
| 3 | Path not found |
| 4 | Invalid/closed descriptor |
| 5 | Corrupt selected file chain |
| 7 | Invalid, unsupported, or oversized UALf |
| 8 | Unsupported syscall |
| 9 | UALf permission denied |

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
