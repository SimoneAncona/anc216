# Executable ANC216 reference profile

This file records decisions needed to resolve gaps in the original PDFs. It describes the tools in this repository, rather than silently changing the PDFs or claiming these choices were already specified there. `common/encoding.hh` is the shared executable encoding reference.

## CPU

- Words and instruction headers are big-endian. PC points to the next instruction. PC-relative offsets use PC **after the entire instruction**, including all operands. Signed offsets and indexing registers use two's complement; low-register writes preserve the high byte.
- Reset clears registers, initializes SP/BP to `0x3000`, PC to `0xff00`, and SR to `0x3c`. Default MTU: IMEM `0..0xfeff`, EMEM `0..0xffff`, user stack `0x3200..0xfeff`. Bounds are inclusive. The default system-SP vector is `0x3000`.
- Absolute user operands are rebased by MTU lower indices. PC/BP-relative addresses, PC/SP/BP, and stack accesses are physical, and user accesses are bounds checked. ROM `0xff00..0xffff` is readable/executable in system mode and never writable through guest stores. Host `load`/`poke` may initialize it.
- Full memory operands are words. LDSR/STSR memory operands are bytes. Unary register operands honor R/L width. Register-to-register encoding permits full registers only. TRSR requires a low register; SP/BP/MTU/CPUID/timer transfers require full registers. TIME accepts immediates as supported by the original assembler.
- ADD/SUB/INC/DEC/NEG/CMP update N/O/Z/C for operand width. Carry means unsigned carry on addition and borrow on subtraction. Logic, LOAD, STORE, TRAN and register POP update N/Z. SIGN changes N only; PAR changes Z only (even parity). Shifts are logical and change carry only; zero shifts retain carry, shifts beyond width produce zero with clear carry. SWAP exchanges two full registers only and leaves flags unchanged; memory and immediate operands are invalid.
- JGE/JGR/JLE/JLS use signed comparisons through N/O, and equality through Z. JLE is `(N != O) || Z`. Numeric jump/call operands in assembly are converted to absolute addressing; immediate jump/call wire encodings are invalid. CALL accepts only absolute addressing; relative, indexed, and indirect CALL encodings are invalid.
- Privileged opcodes trap with NMI code 1. LDSR/POSR/RET cannot elevate S/I/T from user mode. Unknown opcodes/illegal operand combinations trap with code 0; IMEM bounds faults use 2, EMEM mapping faults 3, stack bounds faults 4, soft RESET uses 5. The faulting PC is saved. Unhandled vectors halt with an error instead of looping silently.
- TIME sets a millisecond countdown and stops it. TSTART starts/resumes it; TSTOP stops it. It advances with host steady-clock time, independently of instruction speed. Expiry invokes vector `0x0008` when T is enabled. This is a functional emulator, not cycle-accurate hardware.

### Calls and stack

The stack grows upward; SP is the first free byte. PUSH/POP word order is big-endian. CALL is four bytes: a two-byte instruction header followed by a two-byte absolute target. CALL pushes the next PC (the address after both the header and target operand, two bytes), then SR (one byte), sets BP to the resulting SP, and branches. BP points to the first local byte:

```
BP-3   return PC high
BP-2   return PC low
BP-1   saved SR
BP     first local / start of unused local area
```

RET restores PC/SR from that frame and sets SP to `BP-3`, discarding locals and the return frame. This resolves the PDF's inconsistent SP=BP wording. BP itself is caller-managed. For nested calls, use `phbp; call routine; pobp`. A callee may use BP-relative variables without overwriting its saved return state. There is no SP-relative addressing mode. Register indexing permits only addition (`bp + l4`); `bp - l4` is invalid. A negative signed low-register value can provide a negative displacement. RET is used for routines entered through CALL.

Interrupts follow the document's separate procedure: save old SP at `0x31fe` and old BP at `0x31fc`, load system SP from the word at `0x000c`, set BP=`0x3000`, push PC/SR, enable system mode, and mask external/timer interrupts. NMI additionally pushes old L0 and puts its code in R0. EINR pushes R0/R1/L2 and sets them to device address/data/type. SYSCALL pushes no extra registers. RESET is a soft NMI, not a hard reset. The shared save slots support one interrupt context; nested interrupts require guest-managed saves before re-enabling I/T.

Interrupt handlers must restore their extra saved registers and the interrupted stack explicitly. There is no dedicated RFI opcode. One return strategy is to pop saved SR/PC into scratch registers, load saved SP/BP, push PC/SR onto that restored stack, then use POSR/POPC. Define the scratch-register ABI in the OS. RET is intended for CALL frames, not automatic interrupt return.

### External devices

`--insert address file` maps a read-only memory device. `--insert-card address file` maps a writable MPME memory device. Images are at most 64 KiB. WRITE/HWRITE carry the encoded source data. MPME writes use A=0 to select an internal address and A=1 to send data; transfer width follows the R/L or immediate width. Raw ROM ignores data writes.

READ uses R1 as the outgoing request payload and places the synchronous response in R1. REQ/HREQ also use R1 as payload and invoke EINR when I is enabled, with the documented R0/R1/L2 response convention. Requests made while I is masked currently do not deliver an interrupt; use READ for polling. IREQ is synchronous: R0=device ID, R1=device address; absent devices identify as `0xffff`. Unmapped reads return zero and writes are ignored. Priority variants behave identically because there is no bus contention model.

The emulator accepts raw ROM boot code up to 256 bytes, not an entire UALf program. Larger guest programs can be loaded through mapped external memory by guest firmware, or with the CPU library's host loading API. No BIOS/OS services are built in. Fast mode skips instruction delays; it does not translate guest OS syscalls into host IO.

## AVC64

The display model is 256×224 with RGB332 pixel colors. It is tested without a window. Configure `ANC216_WITH_SDL=ON` to enable an SDL2 window; this requires SDL2 development headers/libraries. Rendering/event handling belongs to the main thread.

The original bus description has no software command packing, so this emulator uses a word with the operation in the high byte and register data in the low byte. Commands 0=pixel, 1=texture, 2=clear, 3=X, 4=Y, 5=D1, 6=D2, 7=D3, 8=D4, 9=NOP. Pixel/clear use D1. READ carries this command word in R1. Texture ID is D1:D2; monochrome textures use D3 foreground and D4 background. Drawing clips to the display.

`--insert-charmap file` loads up to 8192 bytes of concatenated texture records: big-endian ID (2 bytes), width (1), height (1), zero (1), CL (1), then packed pixel data, most-significant bits first, padded to the next byte at each record boundary. CL values 0..5 correspond respectively to 1/2/4/8/8/12 bits per pixel: monochrome; monochrome+1-bit alpha; RGBI 16-color; RGBI+4-bit alpha; RGB332; RGB332+4-bit alpha. Alpha is the low field, zero transparent and maximum opaque. IDs must be unique and dimensions nonzero.

Audio hardware, host keyboard interrupts, custom script extensions, and bus-arbitration timing have no complete software protocol in the supplied documentation and are not implemented. Unsupported extension/default-font options are rejected. `--noaudio`/`--nokeyboard` remain accepted for compatibility. The existing `--fast-mode` flag is a scheduling option, not a host BIOS implementation.

## Confirmed PDF errors

The original ANC216 PDF contains errors: the p. 11 WRITE example uses `0x17`, but the opcode table correctly specifies `0x1b`; the p. 7 `bp - l4` example is not encodable (only addition is supported); and the p. 27 SWAP detail incorrectly allows memory operands. SWAP supports register/register only. See `REVIEW.md` for the full correction list. The PDFs remain the original design documents; this reference records their corrections.

## Assembler and disassembler

The supported assembly language is ANC216.1, including labels, imports/defines/conditional preprocessing, sections, origins, structures, BP-local variables, expressions, byte/word casts, strings and reserves. ANC216.2 preview is rejected explicitly. Output paths are resolved against the invocation directory; imports are resolved relative to their source file and configured import directories.

UALf's base header is 11 bytes. Entry points and symbol offset addresses include the final header size. `-h=ualf` requires `_code`; `-s` includes public symbols and requires that header option. Symbol records are zero-terminated names followed by real address and file offset, both words. The disassembler strips an explicitly selected UALf header and disassembles the payload. It does not reconstruct original source sections, macros, symbol names, or metadata. For raw binary it preserves bytes: unsupported/truncated headers are emitted as byte data, and operand widths are explicit.

## AFS v1 and cardreader

AFS text and old code disagree on field sizes. This implementation preserves the layout of existing repository images:

| Offset | Bytes | Contents |
| --- | ---: | --- |
| 0 | 2 | `fe 01` magic/version |
| 2 | 255 | boot code |
| 257 | 392 | 196 two-byte entries: directory ID, cluster type |
| 649 | 1536 | 96 directories: parent ID + 15-byte name |
| 2185 | 63308 | 196 clusters × 323 bytes |
| 65493 | 43 | reserved |

Directory IDs are 1..96, root is 0. Cluster IDs are 1..196, end-of-chain is 0. Types are 0=free, 1=file head, 2=continuation. All word fields are big-endian; names occupy fixed fields, zero-padded unless they fill the field completely.

A file-head cluster contains a 17-byte base name, 3-byte extension, next ID (1), used payload length (2), and up to 300 data bytes. A continuation contains next ID (1), used length (2), and up to 320 data bytes. An empty file still owns one head cluster. The largest file is 62,700 bytes. The 19-byte name and 297/319-byte payload counts in AFS.txt are inconsistent with existing v1 images and should be corrected, or introduced under a new format version.

The reader validates exact image length, magic/version, directory parents/cycles, duplicate names, cluster types, payload sizes, chain cycles, shared chains, dangling IDs, and orphan continuations. It never fixes or reformats malformed input implicitly. The original `cardreader/test/test.bin` has 16 orphan clusters and is rejected; it was not modified.

Replacing contents checks capacity before changing anything and reuses the file's old clusters. Shrinking/deleting frees the whole continuation chain. Directory deletion is recursive. Paths support root, `.` and `..`; names containing spaces can be quoted in the shell. Read-only commands do not rewrite images. Successful edits save through a temporary sibling file and rename it over the image. This avoids the original implementation's truncation-on-open bug. Writable MPME devices modify emulator memory only; they do not automatically save to the host image.
