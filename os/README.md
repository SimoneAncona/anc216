# ANC216 OS

The kernel is written in ANC216 assembly and split into imported modules. Firmware loads it from external ROM. It mounts AFS v1 on MPME216 chips, runs UALf applications in user mode, and provides console and filesystem syscalls.

## Build and run

From the repository root, with SDL2 development packages installed:

```sh
./build.sh
build/emulator/anc216emu --boot build/os/boot.bin \
    --insert 0x0100 build/os/system.rom \
    --insert-card 0x0200 build/os/disk0.afs \
    --insert-card 0x0201 build/os/disk1.afs \
    --insert-charmap build/os/charmap.bin --gpu=default --uncapped
```

The kernel attempts to run `/bin/init` on chip `0x0200`. The demo tests file reads across three clusters, overwrites a byte, reads the second chip, mounts the first again, and executes `/bin/echo`. Type a line and press Enter. The echo program prints it and exits; the kernel then consumes keys silently while idle. Input echo occurs only during an active `getl` call.

Ctrl+D requests a soft reset. Ctrl+C or closing the SDL window requests guest shutdown. Terminal Ctrl+C also requests shutdown. These controls work independently of the keyboard device. See [the keyboard protocol](../doc/IMPLEMENTATION.md#keyboard-and-host-control-pins).

Without cards, the kernel starts its console and idles. Supply your own AFS images using repeated `--insert-card address image`; the mount syscall selects any chip address. Each chip holds its own filesystem, so additional chips increase available storage without changing AFS's 64 KiB image format. There is one selected volume and one open descriptor at a time; files do not span chips.

`os/build.py` regenerates **only its own demo cards in the output directory**, replacing previous generated copies. Use separate filenames for personal card images. MPME writes currently last for the emulator session; they do not update host image files.

Headless debugging uses the same boot/ROM/card arguments with `--novideo --debug`. `sh info` includes MTU bounds. `start`, `stop`, `soft-reset`, `shutdown`, and `imem watch 0x00f0 8` inspect/control execution. Headless mode has a keyboard device but no SDL text source; the interactive demo waits at `getl`.

## What is system.rom?

`build/os/system.rom` is the kernel packaged for the firmware loader. It is generated from `kernel.anc216`, including all imported modules.

| File offset | Contents |
| --- | --- |
| `0..1` | Unsigned big-endian kernel payload length, including padding |
| `2..` | Raw kernel instructions/data, padded to an even byte count |

The loader accepts nonzero even lengths up to `0x2e00`. There is no magic, AFS, or UALf header. Assembler `org` padding is removed. The file is attached as a device at **EMEM `0x0100`**; firmware copies its payload into **IMEM `0x0100`**, then jumps there. These are separate address spaces. Change both firmware READ operands and the emulator's `--insert` address to relocate the ROM device.

## Source modules and generated files

| File | Responsibility |
| --- | --- |
| `boot.anc216` | Firmware at IMEM `0xff00`; kernel copy loop |
| `kernel.anc216` | Entry, vectors, boot application, reset/shutdown/fault handlers, imports |
| `kernel/console.anc216` | AVC64 character output and persistent cursor |
| `kernel/storage.anc216` | MPME word/byte reads and byte writes |
| `kernel/fs.anc216` | Mount, absolute path lookup, chain validation, sequential read/write |
| `kernel/loader.anc216` | UALf header/symbol bounds, user memory setup, process replacement |
| `kernel/syscalls.anc216` | ABI dispatch, permissions, buffer validation, interrupt return |
| `kernel/keyboard.anc216` | Keyboard IRQ register/frame restoration and silent idle input |
| `programs/init.anc216` | Filesystem/multi-chip/exec demo using actual user syscalls |
| `programs/echo.anc216` | Line input/print/exit demo with a nonzero UALf entry offset |
| `glyphs.txt`, `charmap.bin` | Original printable ASCII glyph source and ready-to-use AVC64 map |
| `build.py` | Assemble firmware/kernel/apps, package ROM, grant demo permissions, generate cards/font |

CMake outputs to `build/os`; invoking `python3 os/build.py` directly outputs to `os/build`. The charmap has 95 monochrome 8×8 records with ASCII IDs 32–126; lowercase uses uppercase letter shapes. Copy a newly generated map to `os/charmap.bin` after editing the glyph source.

## Memory and application ABI

| IMEM | Purpose |
| --- | --- |
| `0000..000d` | Entry/IRQ/NMI/syscall/timer/shutdown vectors and system SP |
| `00f0..00f7` | State, exit code, fault code, last keyboard IRQ payload |
| `0100..2eff` | Kernel code/data |
| `3000..31ef` | Kernel CALL stack; IRQ/syscall stack starts at 3100; interrupted BP/SP saved at `31fc/31fe` |
| `3200..3fff` | UALf staging, maximum entire file size 3584 bytes |
| `4000..7fff` | User memory, with initial SP/BP `7800` |
| `7ff0..7ff3` | Reinstalled POSR/POPC transition stub |
| `ff00..ffff` | Boot ROM |

User absolute operands are logical offsets rebased by MTU `4000`; PC/SP/BP remain physical. Syscall buffers must remain inside logical `0000..3fef`; SP must be `7800..7ffd` to leave room for interrupt return. CALL sites save BP with `phbp; call routine; pobp`.

UALf version 1, ANC216 architecture, application type 0, bounded optional symbol records, and a valid file-relative entry are required. Payloads use logical origin zero. Libraries and symbol-only files are rejected. Flag `80` grants console/input and `20` grants filesystem; other bits are rejected. The demo build sets `a0` for init and `80` for echo; the assembler's default flags grant neither. A failed load keeps the current user memory intact. A successful exec replaces the single process and clears its prior memory.

See [the syscall ABI](SYSCALLS.md) for register arguments and errors.

State at `00f0`: `2160` kernel ready, `2163` user running, `2161` user exited, `2162` user fault, `2164` shutdown, `ffff` kernel panic. Exit status is at `00f2`, fault code at `00f4`, last keyboard IRQ at `00f6`.

## Filesystem scope

AFS uses the corrected v1 layout documented in [IMPLEMENTATION.md](../doc/IMPLEMENTATION.md#afs-v1-and-cardreader). Guest paths are absolute, with nested directories (15-byte names), a 17-byte file base and optional 3-byte extension. Empty components, `.`/`..`, directory dots, and multiple filename dots are rejected. Opening validates IDs, parent/type metadata, payload sizes and cycles in that file's complete chain. It does not audit unrelated files or orphan clusters.

Reads traverse 300-byte head and 320-byte continuation payloads. Writes overwrite existing allocated file bytes and can return a short count at EOF; they do not allocate, resize, create, rename, or delete files. Use cardreader for those operations. Unsupported syscalls return an error. There is no scheduler, dynamic linker, user key callback, allocator, or shell yet.

## Verification

`ctest --test-dir build --output-on-failure` boots the actual ROM/kernel and checks console pixels, real user-mode execution, two MPME volumes, AFS continuation reads/writes, UALf process replacement, keyboard line input, exit, queue/IRQ behavior, SDL event translation, and control-pin handling. SDL tests use the dummy display driver.
