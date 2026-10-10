# Initial ANC216 C ABI

In user mode, PC/SP/BP and object pointers are logical IMEM offsets. The CPU
maps instruction fetches, stack accesses and BP-relative operands through the
IMEM MTU base. System mode uses physical addresses. On a system-to-user status
transition, the CPU converts PC/SP/BP from physical addresses to logical offsets.
Interrupt entry saves the logical user context before selecting the physical
system stack; the kernel prepares return frames physically before switching back.

| Property | Convention |
| --- | --- |
| Byte order | Big-endian |
| `char` | 8 bits, signed by default |
| `short`, `int` | 16 bits |
| `long` | Clang describes 32 bits; backend support is pending |
| Object pointers | 16-bit logical user IMEM offsets (physical in system mode) |
| Alignment | 1 byte for scalar types and pointers |
| Arguments | First two i16 words in R0/R1; remaining words on the stack |
| Return | Void, or one i16 word in R0 |
| Caller-saved | R0..R3 and SR condition flags |
| Callee-saved | R4..R6 |
| Reserved scratch | R7/L7; never allocated to C values |
| Stack | Grows upward; SP points to the first unused byte |
| Stack alignment | 1 byte; SP and BP may be odd |

CALL pushes return PC (2 bytes), then SR (1 byte), and sets BP to the first local
byte. RET restores PC/SR and sets SP to BP-3. BP itself is caller-managed.
Compiled calls expand to:

```text
phbp
call & _function
pobp
```

Saved BP and the CALL frame total five bytes. No alignment padding is required;
local frames use byte alignment. Extra arguments are pushed last-to-first before
saving BP. At callee entry, argument 3 is at BP-7, argument 4 at BP-9, argument 5
at BP-11, and so on. The caller removes these argument words after restoring BP.
These offsets use the logical CALL-frame BP in user mode.

The caller removes its saved BP after RET. Assembly callers use the same
convention as compiled functions.

R4..R6 are preserved through LLVM-generated stack slots when used. R7 is scratch
for frame allocation, unsigned flag tests and arithmetic right shifts. Byte
argument/return values are promoted to an i16 word; ordinary C int promotion is
performed by Clang. Registers L0..L7 overlap the corresponding low byte and writes
preserve the upper byte. Byte memory loads are explicitly extended to word values.

Moves, loads, stores and arithmetic affect flags. Comparisons and their conditional
branches are one machine pseudo so allocation and scheduling cannot insert a
flag-changing instruction between them. Unsigned comparisons capture SR's borrow
bit because ANC216 has no carry-conditional jump.

General pointer loads/stores save BP, set BP to the pointer, access the
object, and restore BP. This requires valid stack space and is slower than a native
register-indirect memory instruction. Functions use a leading `_` in assembly so
C names do not collide with assembler keywords.

The backend appends software 16-bit mul/div/rem helpers when needed. They preserve
R4..R6 and return in R0. Division by zero and signed division overflow retain C's
undefined behavior and have no specified result.

Not yet implemented: 32-bit/aggregate returns,
varargs, indirect calls, floating-point lowering, mutable/non-byte-array global objects, inline assembly,
unwind/debug information, and a standard C library.
Unsupported IR is rejected before instruction selection rather than advertised as
complete ANSI C support.

Constant byte-array globals (including C string literals) are emitted into the
same assembly module. Their pointers are logical addresses in OS executables.
