# Kernel maintenance guide

The kernel is one statically linked, single-process program. Imports divide its
responsibilities without changing the ANC216 CALL ABI. Instructions have C-like
pseudocode comments: `flags = compare(...)` describes the CPU flags, not a C
boolean result; conditional branches name the actual Z/N/O condition. These
comments are explanatory rather than compilable C. Intent and register contracts
appear above routines. `use ... as` defines assembler aliases, not storage.

## Modules

| Module | Contract |
| --- | --- |
| `constants.anc216` | Physical memory layout, vectors, user protection and hardware identities |
| `../libs/abi.anc216` | Shared syscall IDs, record sizes and status codes |
| `../kernel.anc216` | Entry, vector installation, idle loop, faults and shutdown |
| `loader.anc216` | Validate a complete UALf before replacing user memory; reset EMEM grants |
| `syscalls.anc216` | Capture arguments and registers; validate logical user buffers; restore frame |
| `syscall_dispatch.anc216` | One permission policy and indexed table of physical handler addresses |
| `syscall_files.anc216` | Descriptor operations, selected volume and plain exec |
| `syscall_namespace.anc216` | Directory lookup/list/mutation and fstat wrappers |
| `syscall_console.anc216` | Print/getl/clear and direct video grant/revoke |
| `syscall_session.anc216` | Cwd, run arguments and reload init after command exit/fault |
| `fs.anc216` | Import composition; no duplicated filesystem implementation |
| `fs_constants.anc216` | AFS format offsets, dimensions and errors |
| `fs_lookup.anc216` | Mount, absolute-path traversal and file lookup |
| `fs_chain.anc216` | Cluster address/header decoding, complete validation and rewind |
| `fs_transfer.anc216` | Sequential reads and writes with payload/chain growth |
| `fs_namespace.anc216` | Directory enumeration, allocation and removal |
| `fs_stat.anc216` | Payload/allocated usage and bounded recursive parent membership |
| `fs_state.anc216` | Single descriptor, path/header scratch and chain bitmap |
| `storage.anc216` | System-mode MPME address/data protocol helpers |
| `console.anc216` | AVC64 glyphs, coordinates, erase and blinking cursor rendering |
| `keyboard.anc216` | IRQ acknowledgement and interrupted register restoration |

## Calls and syscall flow

Helper callers use `phbp; call helper; pobp`: CALL owns its three-byte PC/SR
frame, while the caller preserves BP. Each helper documents its inputs and
clobbers. Filesystem routines return status in R7 and values/counts in R6.
Shared scratch is safe only because there is one foreground process and user
interrupts remain masked. This code is not a reentrant multiprocess kernel.

Syscall entry captures R0–R5 before using the interrupt stack. The dispatcher
checks the low-byte service number and UALf flags, then uses push/POPC to jump to
a handler without another CALL frame. Handlers finish at `syscall_return`, which
restores the caller's registers and physical SP/BP and returns R6/R7. POSR/POPC
at `0x7ff0` provide the final transition inside the user's allowed IMEM region.
This is distinct from RET, which is used for CALL helpers.

`user_buffer` accepts logical offsets and sizes, checks the entire range below
`0x3ff0`, then adds `USER_BASE`. Device addresses are different: user READ/WRITE
use physical EMEM addresses and do not add the EMEM lower bound. New executables
set lower > upper to deny direct IO. SYS_VIDEO checks console permission and
identity, then grants exactly AVC64; revoke and process replacement remove it.

## Filesystem accounting

FSTAT closes the current descriptor. A file's validated chain supplies logical
payload and complete 323-byte cluster allocation. Directory accounting scans the
fixed directory and file-head tables and walks parents with a 96-step bound;
there is no recursive CALL stack. Totals include descendant files and 16-byte
non-root directory records, excluding global fixed overhead. The ABI record is
specified in [../SYSCALLS.md](../SYSCALLS.md#file-and-directory-disk-usage).

AFS mutation persists through the emulator's MPME backing file. Rebuilding the
OS regenerates its output cards, so personal data should use separately named
images. Regression tests copy attached images before guest writes.
