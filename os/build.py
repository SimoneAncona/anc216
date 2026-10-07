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
        data.extend(code.to_bytes(2, 'big') + bytes([8, 8, 0]))
        data.extend(glyphs[chr(code)])

    return bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assembler', type=Path, default=ROOT.parent / 'build/assembler/assembler')
    parser.add_argument('--cardreader', type=Path, default=ROOT.parent / 'build/cardreader/cardreader')
    parser.add_argument('--output', type=Path, default=ROOT / 'build')
    parser.add_argument('--test-fixtures', action='store_true',
                        help='Also generate regression cards, programs and sample files')
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
    programs = ['sh', 'ls', 'cat', 'touch', 'mkdir', 'rm', 'cd', 'pwd', 'echo', 'clear', 'help', 'mount', 'fstat', 'poweroff', 'lscpu', 'more', 'lsbus', 'redct', 'version']
    programs = sorted(programs)
    print("Building user-space utils")
    for name in programs:
        application = args.output / f'{name}.ual'
        subprocess.run([str(args.assembler.resolve()), str(ROOT / f'programs/{name}.anc216'),
                        str(application.resolve()), '-h=ualf'], check=True)
        data = bytearray(application.read_bytes())
        data[7] = 0x80 if name in ('echo', 'clear') else 0xa0
        if len(data) > 0x0e00:
            raise ValueError(f"{name} exceeds the 3584-byte executable staging region")
        print(f"{name}:\t{len(data)} bytes")
        application.write_bytes(data)
    cardreader = str(args.cardreader.resolve())
    def card(*arguments):
        subprocess.run([cardreader, *map(str, arguments)], check=True,
                       stdout=subprocess.DEVNULL)
    # These are reproducible build artifacts, never a user's attached card.
    disk0 = args.output / 'disk0.afs'
    disk0.unlink(missing_ok=True)
    card('--format', disk0)
    card(disk0, 'mkdir', '/bin')
    card(disk0, 'mkdir', '/data')
    card(disk0, 'mkdir', '/home')
    for name in programs:
        card(disk0, 'put', '/bin/' + name, args.output / f'{name}.ual')
    card(disk0, 'put', '/bin/init', args.output / 'sh.ual')
    card(disk0, 'put', '/data/help.txt', str(ROOT / 'programs/help.txt'))
    if args.test_fixtures:
        disk1 = args.output / 'disk1.afs'
        disk1.unlink(missing_ok=True)
        card('--format', disk1)
        bad = args.output / 'bad.ual'
        bad.write_bytes(b'NOT UALf')
        content = args.output / 'message.txt'
        content.write_bytes(b'A' * 300 + b'B' * 320 + b'C' * 30)
        message = content  # Regression fixture only; production includes only help text in /data.
        content = args.output / 'other.txt'
        content.write_bytes(b'MORE AND CAT TEST, this should work on both of more and cat\n' * 6)
        card(disk1, 'put', '/other.txt', content)
        # Build the integration executable from test sources; never reuse stale artifacts.
        application = args.output / 'init.ual'
        subprocess.run([str(args.assembler.resolve()), str(ROOT.parent / 'tests/os_integration.anc216'),
                        str(application.resolve()), '-h=ualf'], check=True)
        data = bytearray(application.read_bytes())
        data[7] = 0xa0
        application.write_bytes(data)
        # The integration fixture selects the test init without changing disk0's shell.
        demo_disk = args.output / 'demo-disk0.afs'
        demo_disk.write_bytes(disk0.read_bytes())
        card(demo_disk, 'set', '/bin/init', args.output / 'init.ual')
        card(demo_disk, 'put', '/bin/bad', bad)
        card(demo_disk, 'put', '/data/message.txt', message)
        application = args.output / 'readline-demo.ual'
        subprocess.run([str(args.assembler.resolve()), str(ROOT.parent / 'tests/readline_demo.anc216'),
                        str(application.resolve()), '-h=ualf', '-s'], check=True)
        data = bytearray(application.read_bytes())
        data[7] = 0x80
        application.write_bytes(data)
        card(demo_disk, 'put', '/bin/readline-demo', application)

        # Guest-only regression executables live on a separate test image.
        shell_test_disk = args.output / 'shell-test.afs'
        shell_test_disk.write_bytes(disk0.read_bytes())
        card(shell_test_disk, 'put', '/data/message.txt', message)
        # Namespace assertions need a controlled directory independent of help data.
        card(shell_test_disk, 'mkdir', '/namespace-test')
        card(shell_test_disk, 'put', '/namespace-test/message.txt', message)
        for name, data in (
                ('more-lines.txt', b'A\n' * 27 + b'B\n' * 27 + b'C'),
                ('more-wrap.txt', b'A' * (32 * 27) + b'B' * (32 * 27) + b'C'),
                ('more-exact.txt', b'A' * (32 * 27))):
            fixture = args.output / name
            fixture.write_bytes(data)
            card(shell_test_disk, 'put', '/' + name, fixture)
        for test_name in ('strings', 'fs_namespace', 'video'):
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
