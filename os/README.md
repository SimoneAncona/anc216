# ANC216 OS

A first bootable kernel written entirely in ANC216 assembly. It loads from an external ROM, initializes the system stack and fault vectors, prints a banner with an AVC64 console, and stays in an idle loop. This is the starting point for the OS; there is no scheduler, filesystem driver, user-program loader, or keyboard shell yet.

## Build and run

From the repository root, with SDL2 development packages installed:

```sh
cmake -S . -B build -DANC216_WITH_SDL=ON
cmake --build build --parallel
cmake --build build --target os
build/emulator/anc216emu --boot build/os/boot.bin \
    --insert 0x0100 build/os/system.rom \
    --insert-charmap build/os/charmap.bin --gpu=default --speed=100
```

Close the window to exit. Zed also has an **ANC216: run OS (SDL)** task. Do not use `--fast-mode` for the display: that option enables no-video mode.

For the debugger, use the same boot and external ROM with `--novideo --debug`. At `0x00f0`, the kernel writes `0x2160` when ready, or `0xffff` on a fault. `start`, `stop`, and `imem watch 0x00f0 2` inspect this marker. Headless mode runs the same guest but does not attach a display.

## What is system.rom?

`build/os/system.rom` is the kernel packaged for the firmware loader. `os/build.py` generates it from `kernel.anc216`; it is not a source file you need to create manually.

| File offset | Size | Contents |
| --- | --- | --- |
| `0..1` | 2 bytes | Kernel payload length in bytes, unsigned big-endian |
| `2..` | Length from header | Raw kernel instructions and data, padded to an even byte count |

The length includes padding but excludes the two-byte header. There is no magic number, filesystem, or UALf header. The payload begins with the kernel entry point and is assembled for IMEM address `0x0100`; assembler `org` padding is removed before packaging. The current loader accepts nonzero even lengths up to `0x2e00` bytes.

`--insert 0x0100 build/os/system.rom` attaches the file as a read-only device at **EMEM `0x0100`**. Firmware in `boot.bin` reads the header, copies the payload from device offset 2 into **IMEM `0x0100`**, then jumps to that IMEM address. EMEM device addresses and IMEM addresses are separate address spaces; their matching numbers are a convention of this loader.

The three runtime assets have separate jobs:

- `boot.bin`: firmware loaded into IMEM at `0xff00`, at most 256 bytes.
- `system.rom`: kernel image attached at EMEM `0x0100`.
- `charmap.bin`: textures loaded into AVC64's internal storage by `--insert-charmap`; AVC64 commands go to EMEM `0xfffd`.

The bootloader does not discover the ROM address automatically. To change it, change the `read & 0x0100` instructions in `boot.anc216`, rebuild, and use the same address with `--insert`.

## Files and layout

- `boot.anc216`: 101-byte boot ROM at `0xff00`. Reads a big-endian kernel byte count at offset zero of EMEM device `0x0100`, copies its even-sized payload into IMEM at `0x0100`, and jumps there. Zero, odd, or oversized lengths halt the loader.
- `kernel.anc216`: kernel, `puts`/`putc` console routines, and banner. The console uses 32 columns × 28 rows, wraps lines, and clears when the bottom is reached. Keyboard input is pending emulator support.
- `glyphs.txt`: original editable five-by-seven glyphs, centered in eight-by-eight cells. Covers printable ASCII; lowercase currently uses uppercase letter shapes.
- `charmap.bin`: ready-to-use AVC64 map, 95 textures / 1330 bytes. Texture IDs match ASCII codes 32–126. Each record is a big-endian ID, width 8, height 8, zero, CL=0, then eight monochrome row bytes. The console sets foreground white and background black.
- `build.py`: assembles boot/kernel, removes `org` padding from physical-origin payloads, pads the kernel to a word boundary, packages `system.rom`, and generates `charmap.bin` from the glyph source. Outputs go to `build/os` through CMake, or `os/build` when invoked directly. To update the checked-in font after editing glyphs, copy the generated `charmap.bin` into `os/`.

IMEM `0x0000..0x000d` holds vectors/system SP; `0x00f0..0x00f1` is the ready marker; kernel/data starts at `0x0100` and must end before `0x2f00`; the stack starts at `0x3000`. Interrupts remain masked during this initial version; fault/syscall vectors halt through `panic`. CALL sites save BP explicitly. EMEM `0x0100` is the system ROM, and `0xfffd` is AVC64.

The raw system ROM is an initial boot protocol, not an AFS filesystem image. A card-backed filesystem can replace it in a later loader.

## Verification

`ctest --test-dir build --output-on-failure` includes `os_boot`: it boots the actual ROM and kernel, loads the charmap, checks the ready marker and restored stack, verifies pixels in the first banner glyph, and checks continued idle execution without opening a window.
