#!/usr/bin/env python3
"""Assemble the ROM/kernel and generate the original ANC216 OS bitmap font."""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent


def charmap():
    glyphs = {}
    for line in (ROOT / 'glyphs.txt').read_text().splitlines():
        if not line or line.startswith('// '):
            continue
        name, *rows = line.split()
        name = {'SPACE': ' ', 'BACKSLASH': '\\'}.get(name, name)
        values = [int(row, 16) for row in rows]
        if len(name) != 1 or len(values) != 7 or any(row > 31 for row in values):
            raise ValueError(f'Invalid glyph: {line}')
        glyphs[name] = bytes([row << 2 for row in values] + [0])
    data = bytearray()
    for code in range(32, 127):
        data.extend(code.to_bytes(2, 'big') + bytes([8, 8, 0, 0]))
        data.extend(glyphs[chr(code).upper() if chr(code).islower() else chr(code)])
    return bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assembler', type=Path, default=ROOT.parent / 'build/assembler/assembler')
    parser.add_argument('--output', type=Path, default=ROOT / 'build')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    for name in ('boot', 'kernel'):
        subprocess.run([str(args.assembler.resolve()), str(ROOT / f'{name}.anc216'),
                        str((args.output / f'{name}.bin').resolve())], check=True)
    # org emits leading padding; ROM payloads are loaded at their physical origins.
    for name, origin in (('boot', 0xff00), ('kernel', 0x0100)):
        image = args.output / f'{name}.bin'
        data = image.read_bytes()
        if any(data[:origin]):
            raise ValueError('Unexpected data before origin')
        image.write_bytes(data[origin:])
    boot = (args.output / 'boot.bin').read_bytes()
    kernel = (args.output / 'kernel.bin').read_bytes()
    if len(boot) > 256 or not 0 < len(kernel) <= 0x2e00:
        raise ValueError('Boot or kernel exceeds its memory region')
    kernel += bytes(len(kernel) % 2)
    (args.output / 'system.rom').write_bytes(len(kernel).to_bytes(2, 'big') + kernel)
    font = charmap()
    (args.output / 'charmap.bin').write_bytes(font)
    print(f'Boot: {len(boot)} bytes; kernel: {len(kernel)} bytes; font: {len(font)} bytes')


if __name__ == '__main__':
    main()
