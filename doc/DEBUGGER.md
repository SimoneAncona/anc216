# Emulator debugger

Start with `build/emulator/anc216emu --boot boot.bin --debug --novideo`.
Omit `--novideo` to keep the SDL window. Execution starts paused, with the
current instruction displayed. `help` lists commands; existing `ni`, `sh info`,
`imem watch`, `start`, `stop` and `exit` commands remain available.

```text
b 0x0100            # stop before the kernel entry instruction
c                   # continue; console remains responsive
r                   # registers, decoded SR flags and all MTU bounds
u pc 8              # disassemble eight instructions
s                   # execute one instruction, entering calls
n                   # execute one instruction, stepping over CALL/SYSCALL
x sp-16 32          # physical IMEM dump, hexadecimal and ASCII
stack               # dump around SP
```

Enter commands without the example comments. Disassembly marks the current
instruction with `=>` and enabled breakpoints with `*`; it includes raw bytes.
Stops display the reason, instruction count and next instruction. CPU faults
also display their error message. Counts reset on hard reset.

## Breakpoints and execution

| Command | Behavior |
| --- | --- |
| `b <address>` | Set or enable an instruction breakpoint |
| `bl` or `break list` | List enabled and disabled breakpoints |
| `disable <address>` / `enable <address>` | Toggle a breakpoint |
| `delete <address>` | Remove one breakpoint |
| `clear` | Remove all persistent breakpoints |
| `c` / `continue` | Run in the background until a breakpoint, halt or pause |
| `stop` / `pause` | Pause and show the current instruction |
| `s [count]` / `ni [count]` | Execute instructions, ignoring breakpoints |
| `n` / `next` | Step over CALL or SYSCALL; otherwise single-step |
| `until <address>` | Run to a temporary instruction breakpoint |
| `finish` | Run to the return address in the current BP-based CALL frame |

Breakpoints stop **before** execution, including the first instruction of an
interrupt handler. Continuing from a hit executes that instruction once before
checking that breakpoint again, so loops stop on their next iteration.
`next` also checks the original SP at the return address to distinguish nested
calls. Persistent breakpoints still apply during `next`, `until` and `finish`.
A manual pause, step, edit or reset cancels the pending temporary target.

`finish` assumes BP still identifies the current CALL frame, whose return PC is
at BP−3. Use it in CALL routines with the documented BP convention. It does not
unwind interrupt frames. A syscall that replaces or exits a program may never
return to the target selected by `next`; pause with `stop` or terminal Ctrl+C.

## Inspecting and editing

`devices` lists connected devices with their physical EMEM address, type and
identification ID. This does not read device data or change device state.

`r` shows R0–R7, PC, SP, BP, SR, decoded flags, the last instruction header and
MTU IMEM/EMEM/stack bounds. `status` shows execution state. `u [address] [count]`
disassembles from PC by default. `x <address> [bytes]` dumps physical IMEM,
defaulting to 64 bytes. `stack [bytes]` starts up to 32 bytes below SP.

Numbers are decimal or `0x` hexadecimal; bare hexadecimal containing letters
such as `ff00` is accepted. Use `0x0100` for address 256. Address expressions
accept `pc`, `sp`, `bp`, `r0`–`r7`, `l0`–`l7` and one `+` or `-` offset without
spaces, such as `bp-3`. Ranges outside physical IMEM are rejected.

Debugger instruction addresses, breakpoints and memory dumps are physical.
For user code, add the MTU IMEM base to an assembled logical address. For example,
logical address `0x0020` with the OS base `0x4000` means `b 0x4020`.
Memory inspection takes a locked snapshot; inspecting a running CPU does not
pause it, so separate commands can describe different execution moments.

`set r0 0x1234`, `set l0 0xab`, `set pc 0x0100`, `set sp 0x3000`,
`set bp 0x3000` and `set sr 0x0a` pause and edit CPU state. Low-register edits
preserve the upper byte. `set mem 0x0100 0x00` pauses and edits one physical
IMEM byte. SR and low registers accept only byte values; edits do not calculate
arithmetic flags.

## Terminal and machine controls

On Unix terminals, arrow keys edit the line and browse history. Home/End,
Delete/Backspace, Ctrl+A/Ctrl+E and Ctrl+U provide normal line editing. Tab
completes command names. `history` lists commands and `!3` repeats command 3.
An empty line repeats the latest step, next or inspection command.

Terminal **Ctrl+C pauses** in debug mode. Terminal Ctrl+D on an empty line exits.
In the SDL window, Ctrl+D requests the guest soft-reset pin and Ctrl+C requests
the guest shutdown pin; `soft-reset` and `shutdown` do the same from the console
and resume the CPU so it can handle them. `q` / `exit` exits the emulator directly.
`reset` resets CPU state and instruction count, preserving loaded memory and
persistent breakpoints. It does not reload disk images or reset every device.

On Unix, SDL input and stop notifications continue while the console waits.
Other platforms use line input and poll SDL between commands.
