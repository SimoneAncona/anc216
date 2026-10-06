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
        data.extend(glyphs[chr(code)])
    return bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assembler', type=Path, default=ROOT.parent / 'build/assembler/assembler')
    parser.add_argument('--cardreader', type=Path, default=ROOT.parent / 'build/cardreader/cardreader')
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
    # Shell and commands are real user executables; grants follow their services.
    programs = ('init', 'readline-demo', 'sh', 'ls', 'cat', 'touch', 'mkdir', 'rm', 'cd', 'pwd', 'echo', 'clear', 'help', 'mount', 'poweroff')
    for name in programs:
        application = args.output / f'{name}.ual'
        subprocess.run([str(args.assembler.resolve()), str(ROOT / f'programs/{name}.anc216'),
                        str(application.resolve()), '-h=ualf', '-s'], check=True)
        data = bytearray(application.read_bytes())
        data[7] = 0x80 if name in ('echo', 'clear', 'help', 'readline-demo') else 0xa0
        if len(data) > 0x0e00:
            raise ValueError(f"{name} exceeds the 3584-byte executable staging region")
        application.write_bytes(data)
    cardreader = str(args.cardreader.resolve())
    def card(*arguments):
        subprocess.run([cardreader, *map(str, arguments)], check=True,
                       stdout=subprocess.DEVNULL)
    # These are reproducible build artifacts, never a user's attached card.
    for name in ('disk0.afs', 'disk1.afs'):
        (args.output / name).unlink(missing_ok=True)
        card('--format', args.output / name)
    disk0, disk1 = args.output / 'disk0.afs', args.output / 'disk1.afs'
    card(disk0, 'mkdir', '/bin')
    card(disk0, 'mkdir', '/data')
    bad = args.output / 'bad.ual'
    bad.write_bytes(b'NOT UALf')
    card(disk0, 'put', '/bin/bad', bad)
    for name in programs:
        card(disk0, 'put', '/bin/' + ('demo-init' if name == 'init' else name), args.output / f'{name}.ual')
    card(disk0, 'put', '/bin/init', args.output / 'sh.ual')
    content = args.output / 'message.txt'
    content.write_bytes(b'A' * 300 + b'B' * 320 + b'C' * 30)
    card(disk0, 'put', '/data/message.txt', content)
    content = args.output / 'other.txt'
    content.write_bytes(b'SECOND MPME VOLUME OK!\n')
    card(disk1, 'put', '/other.txt', content)
    # The integration fixture selects the old init without changing disk0's shell.
    demo_disk = args.output / 'demo-disk0.afs'
    demo_disk.write_bytes(disk0.read_bytes())
    card(demo_disk, 'set', '/bin/init', args.output / 'init.ual')
    # Guest-only regression executables live on a separate test image.
    shell_test_disk = args.output / 'shell-test.afs'
    shell_test_disk.write_bytes(disk0.read_bytes())
    for test_name in ('strings', 'fs_namespace'):
        application = args.output / f'test-{test_name}.ual'
        subprocess.run([str(args.assembler.resolve()), str(ROOT.parent / f'tests/{test_name}_test.anc216'),
                        str(application.resolve()), '-h=ualf', '-s'], check=True)
        data = bytearray(application.read_bytes())
        data[7] = 0xa0
        application.write_bytes(data)
        card(shell_test_disk, 'put', '/bin/test-' + test_name, application)
    font = charmap()
    (args.output / 'charmap.bin').write_bytes(font)
    print(f'Boot: {len(boot)} bytes; kernel: {len(kernel)} bytes; font: {len(font)} bytes')


if __name__ == '__main__':
    main()
