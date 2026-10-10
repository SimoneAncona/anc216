# ANC216

A small 16-bit architecture for educational and microcontroller experiments.

Experimental LLVM backend setup lives in [llvm/](llvm/README.md). It currently
provides target registration and initial register/instruction descriptions;
C-to-ANC216 code generation is not implemented yet.

The repository contains an assembler, a lossless raw-binary disassembler, a functional CPU emulator, and an AFS v1 card-image reader/editor. All CPU opcodes are implemented, with shared instruction validation and tests for arithmetic flags, addressing, stack frames, protection, interrupts, timers, IO, and AVC64 device operations.

Read [the ISA/documentation review](doc/REVIEW.md) for the design assessment and [the implementation profile](doc/IMPLEMENTATION.md) for precise semantics and corrections to the original PDFs. The core design is clean; the stack/interrupt ABI and specialized IO/addressing modes need the most specification work before building an OS.

## Build and test

Requires a C++20 compiler, CMake 3.16+, and Python 3 for integration tests. SDL is optional.

```sh
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

`./build.sh` and the PowerShell build script enable SDL by default and require SDL2 development headers/libraries. For a headless build, run `./build.sh -DANC216_WITH_SDL=OFF`. Direct CMake configuration keeps SDL optional; enable it with `-DANC216_WITH_SDL=ON`.

## Production build

```sh
./build.sh --release
./runos.sh
```

The production pipeline builds optimized Release binaries in `build/production`
with `BUILD_TESTING=OFF`: no test executables, test runs, or OS regression fixtures.
The OS contains only `disk0.afs`, with the shell and commands in `/bin` and help text
in `/data/help.txt`. It generates no `message.txt`, `other.txt`, or disk1.
`runos.sh` boots this build with only disk0 attached and forwards emulator options.
Use `./build.sh --release -DANC216_WITH_SDL=OFF` for a headless build, then
`./runos.sh --novideo --debug`. Windows builds use `./build-production.ps1`.
`build.sh` without `--release` and `build.ps1` run the development build/test pipeline.
`build-production.sh` remains an alias for `build.sh --release`.

## Zed

Project settings live in `.zed/settings.json`. CMake generates `build/compile_commands.json`, and `.clangd` points to it so each tool gets its own custom-header include path. Configure once with `cmake -S . -B build`, then restart the C++ language server if the project was already open. This follows [Zed's C++ compilation database setup](https://zed.dev/docs/languages/cpp).

The root `.clang-format` uses Allman braces (opening braces on new lines), four spaces, and preserves include order. Zed formats C++ through clangd, which reads this file automatically.

Use `task: spawn` for the build/test tasks or **ANC216: debug boot (no video)**. The latter assembles the example and opens the emulator's interactive debugger in Zed's terminal.

## Assemble, inspect, and run

```sh
build/assembler/assembler examples/boot.anc216 boot.bin
build/disassembler/disassembler boot.bin boot.dis.anc216 --stdout
build/emulator/anc216emu --boot boot.bin --novideo --fast-mode --max-cycles=100
build/emulator/anc216emu --boot boot.bin --novideo --debug
```

In the debugger, `b 0xff00` sets a breakpoint, `c` continues, `s` steps, and `n` steps over calls. `r` shows registers and MTU bounds; `u pc` disassembles and `x sp-16 32` inspects memory. See the [debugger guide](doc/DEBUGGER.md) for editing, history and machine controls. The example leaves R1=`0x000f`.

Boot images are raw code at `0xff00`, at most 256 bytes. PC-relative labels work in the ROM example. Absolute ROM labels must include `0xff00` because the assembler's raw origin defaults to zero. Guest firmware must load larger programs and supply its own BIOS/OS services. The OS in [os/](os/README.md) includes modular kernel code, MPME/AFS access, user syscalls, a UALf loader and an AVC64 text console. Its generated kernel image, `build/os/system.rom`, uses a two-byte length header followed by the kernel; see [the OS ROM format and memory mapping](os/README.md#what-is-systemrom).

UALf output: `assembler program.anc216 program.ualf -h=ualf -s`; it requires an `_code` label and `-s` optionally includes public symbols. Disassemble its payload with `-h=ualf`.

## Create and edit a card

```sh
build/cardreader/cardreader --format card.bin
build/cardreader/cardreader card.bin mkdir bin
build/cardreader/cardreader card.bin put /bin/boot.bin boot.bin
build/cardreader/cardreader card.bin ls /bin
build/cardreader/cardreader card.bin get /bin/boot.bin extracted.bin
build/cardreader/cardreader card.bin
```

The interactive shell supports `mkdir`, `cd`, `ls`, `touch`, `set`, `put`, `get`, `find`, `du`, `rm`, `boot`, and `exit`. Quote paths containing spaces. Successful edits save on exit/EOF; one-command invocations save immediately. Invalid images are rejected without modification.

Attach a card to the emulator with `--insert-card 0x0100 card.bin`, or a read-only flat ROM range with `--insert 0x0100 file.bin` (each file byte occupies an EMEM address; the range must not overlap other mappings). MPME writes update and flush the attached host file immediately. Use copies for experiments you want to discard. The supplied old card fixture has orphan clusters; create a fresh card for new experiments.

The emulator is functional rather than cycle accurate. Audio, script extensions and bus contention are not implemented; their protocols are not fully specified in the supplied documents. AVC64's pixel/texture model is implemented and tested; its optional SDL window requires a display environment.

## Keyboard and window

The SDL window defaults to 1280×1120 (5× AVC64 pixels). Use `--zoom=2` for 512×448, or any integer 1–16.

SDL captures printable ASCII and control/special keys. The keyboard device is EMEM `0xfffc`; queued events can be polled with READ or delivered through EINR. Ctrl+D requests a guest soft reset; Ctrl+C/window close requests guest shutdown. See [the complete keyboard protocol](doc/IMPLEMENTATION.md#keyboard-and-host-control-pins), including queue commands, IRQ registers and masking behavior.

The debugger's `sh info` includes MTU IMEM, EMEM and stack bounds. `soft-reset`/`shutdown` request guest control pins; `reset` resets CPU state.

## Emulator speed

Use `--uncapped` to run as fast as the host allows while keeping video and keyboard input. `--speed=N` targets approximately `100 × N` instructions/second using batched pacing; `--fast-mode` also removes delays but disables video/audio. SDL input/rendering updates about 60 times per second.

For optimized binaries, build with `./build.sh -DCMAKE_BUILD_TYPE=Release`. This keeps SDL enabled and runs the tests.
