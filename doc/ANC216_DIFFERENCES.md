# Differences and clarifications relative to ANC216.pdf

This file collects the corrections, implementation choices and open questions
we have discussed. The original PDF remains unchanged. A clarification fills a
gap; it does not imply that the PDF explicitly specifies the opposite behavior.
See [IMPLEMENTATION.md](IMPLEMENTATION.md) for the full executable profile and
[REVIEW.md](REVIEW.md) for the architecture review.

## Confirmed corrections and agreed interpretation

| PDF location | Issue | Implemented rule |
| --- | --- | --- |
| pp. 5–7 | Text calls some modes SP-relative, while the encodings use BP. | These modes are BP-relative; no SP-relative addressing exists. |
| p. 5 | System SP storage is described as one byte. | The 16-bit system SP occupies `0x000c..0x000d`; subsequent OS storage starts at `0x000e`. |
| p. 8 | A saved word SP is placed at the final byte `0x31ff`. | Saved SP occupies `0x31fe..0x31ff`; saved BP occupies `0x31fc..0x31fd`. |
| pp. 14, 23 | CALL/RET frame cleanup and return position are ambiguous. | CALL accepts only absolute addressing and saves PC after its complete four-byte instruction, then SR. BP becomes the first local byte. RET restores PC/SR and sets SP to BP−3; BP preservation is caller-managed. RET returns from CALL routines. |
| p. 18 | JLE uses AND between less-than and equality conditions. | JLE tests `(N != O) OR Z`. |
| pp. 14, 21 | Additional-request flag A is confused with Z. | A is SR bit 2 (`0x04`); Z is a separate flag. |
| p. 11 | WRITE example uses opcode `0x17`. | The opcode table is correct: WRITE is `0x1b`; example bytes are `e0 1b ff 00`. |
| p. 7 | `bp - l4` implies a subtraction encoding. | Only register addition is encoded: `bp + l4`. A signed negative low-byte index can produce a negative displacement. |
| p. 27 | SWAP detail permits memory operands. | SWAP exchanges two full registers only. |


## Emulator clarifications and limitations

- **MTU and S:** S=0 rebases absolute IMEM operands using the IMEM lower
  index and enforces user bounds. EMEM addresses are physical and bounds checked,
  without rebasing, in every IO addressing mode. S=1 uses physical addresses and bypasses user
  bounds. PC/SP/BP, PC/BP-relative addresses and stack accesses are physical;
  user accesses still undergo their applicable bounds checks. Interrupt/syscall
  entry sets S; restoring SR with POSR can resume user mode. This explicit
  execution contract resolves ambiguity in the PDF's general MTU description.
- **Reset defaults:** the emulator initializes PC=`0xff00`, SP/BP=`0x3000`,
  SR=`0x3c`, IMEM bounds `0..0xfeff`, EMEM bounds `0..0xffff`, stack bounds
  `0x3200..0xfeff`, and the system-SP vector to `0x3000`.
- **Return ABI:** CALL pushes a two-byte PC and one-byte SR; BP points just above
  that frame. Nested callers preserve BP with `phbp; call routine; pobp`.
  Interrupt return restores saved registers and SP/BP explicitly, then uses
  POSR/POPC. There is no new interrupt-return opcode. NMI saves only L0, so the
  original high byte of R0 is not automatically preserved.
- **Instruction details:** PC-relative offsets use PC after the entire
  instruction. Signed offsets/indexes use two's complement. Low-register writes
  preserve the upper byte. LDSR/STSR memory transfers are bytes; ordinary full
  memory transfers are words. TIME accepts immediate operands as the original
  assembler did. Explicit flag behavior, shift edge cases and operand legality
  are recorded in IMPLEMENTATION.md and the shared encoding table.
- **Protection:** user LDSR/POSR/RET cannot elevate S/I/T. Faults save the faulting
  PC; an unhandled vector halts with an error. Guest stores cannot modify ROM
  `0xff00..0xffff`; host loading/debug initialization can.
- **Timer:** TIME loads a stopped millisecond countdown, TSTART starts/resumes,
  and TSTOP stops it. Time follows the host steady clock, independent of guest
  instruction speed. The emulator does not model cycle-accurate hardware timing.
- **Device requests:** READ uses R1 as request and response. REQ/HREQ use R1 as
  payload and deliver EINR only when I is enabled; masked requests are not queued
  for later delivery. IREQ returns ID in R0 and address in R1. An absent device
  identifies as `0xffff`; unmapped reads return zero and writes do nothing.
  High-priority variants have no scheduling distinction without bus arbitration.
- **Mapped ROM/MPME:** WRITE/HWRITE carry their encoded source data. MPME uses
  A=0 to select its internal address and A=1 to write data, with byte/word transfer
  width. MPME writes update and flush the attached host image immediately.
- **Keyboard:** the ASCII/scancode queue, polling requests, overflow handling and
  keyboard IRQ payload are an emulator protocol supplement, rather than a
  complete protocol specified by ANC216.pdf. The device is at EMEM `0xfffc`, ID
  `0x0301`. See IMPLEMENTATION.md for the complete wire contract.
- **Host controls:** SDL Ctrl+D requests soft RESET NMI; SDL Ctrl+C/window close
  requests shutdown. Terminal Ctrl+C requests shutdown during normal execution
  and pauses during debugging. These are host bindings to guest control pins.
- **Unsupported hardware:** audio, custom script extensions and bus-arbitration
  timing lack complete implemented software contracts.

## Accepted revision: user-mode IO and EMEM bounds

The PDF marks READ and WRITE privileged. At the author's request, the emulator
now permits these two instructions in user mode. Their encodings are unchanged.
All user IO addresses, including register and PC/BP-relative forms, are checked
against inclusive EMEM lower/upper bounds and remain **absolute physical device
addresses**. There is no EMEM rebasing. An out-of-range address faults with code 3.
System mode still bypasses these bounds.

IREQ, REQ, HREQ, HWRITE, PAREQ, CAREQ and MTU setters remain privileged. User SR
writes can still change A while preserving protected S/I/T; this existing rule
allows a granted MPME device's address/data protocol through WRITE. The OS grants
only AVC64 through SYS_VIDEO; it starts each process with an empty interval.
A grant permits all READ/WRITE commands on every device in the interval, rather
than selected files or operations. The OS uses a single-address video interval.
See [the syscall ABI](../os/SYSCALLS.md#direct-video-access).

## OS conventions, not ISA changes

The kernel at `0x0100`, flat ROM at EMEM `0x0100`, boot cards at `0x3000/0x3001`,
user IMEM `0x4000..0x7fff`, interrupt stack at `0x3100`, and return stub at
`0x7ff0` are this OS's choices. POSR at `0x7ff0` restores SR; POPC at `0x7ff2`
restores the actual user PC. The stub must be inside user IMEM after clearing S.
It is not a new ISA instruction or a fixed architectural return address.

The initial OS implements selected syscalls and adds mount/exec/rewind/size;
it does not implement every proposed service from the introductory PDF.
User input is polled by getl with interrupts masked; idle IRQs consume keys
silently. See [the OS guide](../os/README.md) and [ABI](../os/SYSCALLS.md).

## Related documents

The previously discussed UALf 11-byte base header/file-relative entry, AFS v1
17+3-byte filenames and 300/320-byte payloads, and AVC64 256×224 resolution and
emulator command/texture packing concern their own format/device documents,
rather than ANC216.pdf alone. Their corrections and implementation contracts
remain in IMPLEMENTATION.md and REVIEW.md.

## Corrected emulator ROM mapping

The earlier emulator incorrectly gave ROM an MPME-like internal-offset protocol:
it read different bytes from one EMEM address using R1. This was an invented
behavior, not an ANC216.pdf rule. It has been removed. ROM now maps consecutive
EMEM bytes; firmware advances the bus address, and complete mapping ranges are
checked for overlap. The OS length-prefixed ROM format and origin `0100` remain
OS conventions; the PDF's example reset loader uses EMEM `0000..2fff`.

## IREQ

IREQ is not a system priv. instruction anymore, as opposed to the PDF, and
additionally, unlike write and read, doesnt follow the MTU bound
