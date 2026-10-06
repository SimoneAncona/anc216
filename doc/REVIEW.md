# ISA and documentation review

ANC216 has a clean, understandable core for an educational microcontroller. Eight 16-bit registers with named low bytes, big-endian words, an explicit addressing byte, and conventional arithmetic/condition flags make it easy to teach and decode. A single system/user bit is reasonable; a ring hierarchy would add little here. Software multiplication and division are also reasonable for a small implementation.

The encoding is regular, although instructions are variable length: two header bytes followed by zero to four operand bytes. Separating addressing from the operation is a good design choice. The main implementation risk was three different copies of the ISA drifting apart; `common/encoding.hh` now supplies opcode names, legal operand families, privilege requirements, and wire decoding.

## What I would simplify before a hardware implementation

1. **Addressing modes:** immediate-to-memory and the many BP/PC variants create considerable decoder complexity for a small machine. Keep them if they are useful to you, but measure the cost against the assembly saved. Ordinary register-indirect data loads/stores are more useful for microcontrollers than several of the specialized variants. Currently `load r0, [r1]` cannot be encoded; general pointer walking is awkward.
2. **IO operations:** give `req`, `read`, `ireq`, `hreq`, `write`, `hwrite`, and the additional flag an exact software contract, including request payloads and response registers. Higher priority has no observable effect until there is actual bus contention.
3. **Stack ABI:** a frame diagram and a tested nested-call example would remove the most serious ambiguity. Specify how callers preserve BP, how local variables relate to it, and how an interrupt restores the interrupted SP/BP and privilege state.
4. **Protection:** MTU rebasing can be useful for small kernels, but the document should separate physical PC/SP/BP values from logical absolute operands. The EMEM lower/upper indices are currently redundant; see the unresolved issue below.

I would stabilize these details before adding instructions. The lack of rings is not a weakness. The current omissions matter more for predictable compiler/firmware behavior than for desktop CPU features.

## Unresolved: EMEM MTU bounds and privileged IO

The current rules prevent the EMEM lower/upper indices from controlling device
access. IO instructions (`ireq`, `req`, `hreq`, `read`, `write`, `hwrite`) require
system privileges. In user mode (S=0), the emulator raises a privilege fault
before translating the EMEM address. In system mode (S=1), IO is permitted but
MTU rebasing and user bounds checks are bypassed. The EMEM indices can be set
and inspected, but neither execution mode uses them to restrict device access.

A future ISA decision is needed. Possible resolutions are:

- Allow selected IO instructions in user mode, applying EMEM rebasing and bounds
  checks so the kernel can grant access to a device range. Define which operations
  are permitted and whether a contiguous range provides sufficient isolation.
- Keep IO privileged and remove or explicitly reserve the unused EMEM indices.
- Define a separate rule that applies EMEM translation/bounds to selected system
  IO operations. Specify how the kernel accesses physical devices outside that
  range and how it changes the active mapping.

No resolution has been selected. This review entry does not change the ISA,
emulator behavior or kernel; device access still goes through privileged kernel
code and syscalls.

## Documentation assessment

The architecture PDF is a good introduction and a useful design record. The assembly tutorial has concrete examples, and the encoding and opcode tables are helpful. It is not yet precise enough to be an unambiguous implementation specification. Preserve the explanatory material, then add a concise normative reference with instruction widths, exact flag effects, operand legality, frame diagrams, and executable examples.

The first nine corrections below have been confirmed by the ISA author. BP-relative addressing is intentional; there is no SP-relative addressing mode. CALL accepts only an absolute target, and RET is the return instruction for routines entered through CALL.

| Source | Issue | Resolution in this implementation |
| --- | --- | --- |
| ANC216 pp. 5–7 | Text says SP-relative while encodings say BP-relative. | BP-relative. |
| ANC216 p. 5 | System SP occupies one byte although SP and the system stack addresses are 16-bit. | Word at `0x000c..0x000d`; OS storage starts at `0x000e`. |
| ANC216 p. 8 | A word SP is saved in the single final byte `0x31ff`. | SP at `0x31fe..0x31ff`; BP saved at `0x31fc..0x31fd`. |
| ANC216 pp. 14, 23 | CALL/RET descriptions do not explain removal of their three-byte return frame or restoration of BP. | CALL uses absolute addressing only. RET resumes after the CALL header and address operand; explicit frame/ABI in `IMPLEMENTATION.md`. |
| ANC216 p. 18 | JLE uses `N != O AND Z == 1`, which does not implement less-or-equal. | `(N != O) OR Z`. |
| ANC216 pp. 14, 21 | Additional request flag A is described as Z. | A is bit 2, independent of zero. |
| ANC216 p. 11 | WRITE example calls its opcode `0x17`. | The opcode table is correct: WRITE is `0x1b`. The PDF example is wrong; corrected bytes are `e0 1b ff 00`. |
| ANC216 p. 7 | Example `bp - l4` implies an unencoded subtraction bit. | The PDF example is wrong: only `bp + l4` is encodable. The low byte is signed; subtract-register syntax is rejected. |
| ANC216 p. 27 | SWAP detail allows memory, summary/software only allowed registers. | The PDF detail allowing memory is wrong. SWAP exchanges two full registers only; memory and immediate operands are invalid. |
| UALf | Header size and entry offsets were incomplete in the code. | Fixed 11-byte base header, bounded symbol records, file-relative entry point. |
| AFS.txt | Filename length and payload capacities disagree with code and existing images. | Preserve existing AFS v1 layout: 17-byte base, 3-byte extension, payloads 300/320 bytes. |
| avc64.txt | Resolution alternates between `224 * 256` and code's `256 * 254`; command packing and CL codes are unspecified. | 256×224; documented emulator command/texture encoding. |

NMI pushes only L0 but sets the code in R0, so its original high byte is not automatically preserved. That is another ABI detail worth deciding explicitly before building an OS.

System calls and memory maps in the introductory document are proposed OS conventions, rather than requirements for every microcontroller program. The emulator executes real guest handlers; it does not pretend that the listed OS services already exist.
