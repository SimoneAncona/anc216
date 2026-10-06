#!/usr/bin/env python3
"""Black-box regression tests: wire format, CLI, boot, and AFS persistence."""
import pathlib
import random
import subprocess
import sys
import tempfile

assembler, disassembler, emulator, cardreader = sys.argv[1:]

def run(*args, ok=True, input=None):
    result = subprocess.run([str(a) for a in args], input=input, capture_output=True, timeout=10)
    if ok and result.returncode:
        raise RuntimeError(f'{args} returned {result.returncode}: {result.stdout!r} {result.stderr!r}')
    if not ok and not result.returncode:
        raise RuntimeError(f'Unexpected success: {args}')
    return result

with tempfile.TemporaryDirectory(prefix='anc216-tests-') as directory:
    p = pathlib.Path(directory)
    source, binary, restored = p/'source.anc216', p/'source.bin', p/'restored.bin'
    def assemble(text, ok=True, *options):
        source.write_text(text)
        return run(assembler, source, binary, *options, ok=ok)
    def roundtrip(data):
        binary.write_bytes(data)
        run(disassembler, binary, source)
        run(assembler, source, restored)
        if restored.read_bytes() != data:
            raise RuntimeError(f'Round-trip mismatch; source preserved in /tmp/anc216-failed.anc216')

    # Every possible two-byte header, including reserved/invalid combinations.
    # Padding exercises all operand widths; the next record must remain intact.
    for addressing in range(256):
        data = b''.join(bytes([addressing, op, 0xf8, 0x90, 0x12, 0x34]) for op in range(256))
        try: roundtrip(data)
        except Exception:
            pathlib.Path('/tmp/anc216-failed.anc216').write_text(source.read_text())
            print('Failing addressing byte:', hex(addressing))
            raise
    for size in range(1,9):
        for addressing in (0,8,16,3,11,6,14,7,0xc0,0xc1,0xc5,0xc6,0x84,0x82):
            roundtrip(bytes([addressing,0x3b,0xff,0xff,0xff,0xff,0xff,0xff])[:size])
    rng = random.Random(216)
    roundtrip(bytes(rng.randrange(256) for _ in range(8192)))

    assemble('load r0, (word 5) + (byte 2)\n')
    assert binary.read_bytes() == bytes.fromhex('c13a0007')
    assemble('reserve 3\nbyte 0x55\n')
    assert binary.read_bytes() == bytes.fromhex('00000055')
    assemble('reserve -1\n', False)
    (p/'part.anc216').write_text('byte 0xbb\n')
    assemble('byte 0xaa\nimport "part.anc216"\nbyte 0xcc\n')
    assert binary.read_bytes() == bytes.fromhex('aabbcc')
    # Imported libraries share use/as aliases with their callers and siblings.
    (p/'constants.anc216').write_text('use SHARED_BYTE as 0x42\n')
    (p/'alias-user.anc216').write_text('byte SHARED_BYTE, CALLER_BYTE\n')
    assemble('use CALLER_BYTE as 0x24\nimport "constants.anc216"\nimport "alias-user.anc216"\nbyte SHARED_BYTE\n')
    assert binary.read_bytes() == bytes.fromhex('422442')
    assemble('use enabled\nif enabled then\nkill\n', False)
    assemble('tran l0, l1\n', False)
    assemble('swap r0, r1\n')
    assert binary.read_bytes() == bytes.fromhex('413d')
    for text in ('swap r0, & 0x4000\n', 'swap r0, 1\n', 'seli & bp - l4\n'):
        assemble(text, False)
    assemble('seli & bp + l4\nwrite & 0xff00, r4\n')
    assert binary.read_bytes() == bytes.fromhex('a652e01bff00')
    assemble('call 0x1234\n')
    assert binary.read_bytes() == bytes.fromhex('80041234')
    for text in ('call * 0\n', 'call & bp + 2\n', 'call ** 0x1234\n'):
        assemble(text, False)
    assemble('load r0, (5 + 7) * 2\n')
    assert binary.read_bytes() == bytes.fromhex('c13a0018')
    assemble('start:\n jmp end\n byte 0x55\nend:\n kill\n')
    assert binary.read_bytes() == bytes.fromhex('80220005550000'), 'small jump target changed label offsets'
    assemble('load r0, -(1 + 2)\nload l1, byte (8 / 2)\n"a;b", 0b1010\n')
    assert binary.read_bytes() == bytes.fromhex('c13afffdcd3a04613b620a')
    assemble('store & bp - 2, byte 0xaa\nstore & bp + 4, word 0x1234\n')
    assert binary.read_bytes() == bytes.fromhex('033bfeaa0b3b041234'), 'BP immediate store duplicate arguments'
    assemble('jmp * far\nreserve 200\nfar:\nkill\n', False)
    for text in ('load r0, 1 / 0\n', 'load r0, 65536\n', 'store & 0x4000 + l1, word 0x1234\n', 'load r0, missing\n', '"unterminated', 'if missing\nkill\n'):
        assemble(text, False)

    assemble('data:\nbyte 0xaa\n_code:\ncpuid\nkill\n', True, '-h=ualf', '-s')
    ualf = binary.read_bytes()
    header_size = int.from_bytes(ualf[9:11], 'big')
    assert ualf[:4] == b'UAL\x01' and int.from_bytes(ualf[4:6], 'big') == header_size+1
    run(disassembler,binary,source,'-h=ualf')
    run(assembler,source,restored)
    assert restored.read_bytes() == ualf[header_size:]
    for bad in (b'UAL', b'UAL\x01\0\0\x01\0\0\0\x0a', b'UAL\x01\0\0\x01\0\0\0\x20name'):
        binary.write_bytes(bad); run(disassembler,binary,source,'-h=ualf',ok=False)

    assemble('ldsp 0x3000\nload r0, 5\nload r1, 0\nloop:\nadd r1, r0\ndec r0\ncmp r0, 0\njne * loop\nkill\n')
    run(emulator,'--boot',binary,'--novideo','--fast-mode','--max-cycles=100')
    debug = run(emulator,'--boot',binary,'--novideo','--debug',input=b'ni\nni\nni\nsh info\nexit\n')
    assert b'MTU IMEM=0000..feff EMEM=0000..ffff STACK=3200..feff' in debug.stdout
    assert b'R0=0005' in debug.stdout and b'SP=3000' in debug.stdout
    run(emulator,'--boot',ok=False)
    run(emulator,'--boot',binary,'--speed=nan',ok=False)
    for zoom in ('0', '17', 'nan', '-1'):
        run(emulator,'--boot',binary,'--zoom='+zoom,ok=False)
    run(emulator,'--boot',binary,'--novideo','--uncapped','--zoom=3')
    run(emulator,'--boot',binary,'--novideo','--fast-mode','--zoom','5')
    run(emulator,'--boot',binary,'--unknown',ok=False)
    binary.write_bytes(bytes(257)); run(emulator,'--boot',binary,ok=False)
    binary.write_bytes(bytes.fromhex('8022ff00')); run(emulator,'--boot',binary,'--fast-mode','--max-cycles=5',ok=False)

    image = p/'card.bin'; payload = p/'payload.bin'; exported = p/'exported.bin'
    run(cardreader,'--format',image)
    assert image.stat().st_size == 65536
    run(cardreader,image,'mkdir','bin')
    run(cardreader,image,'mkdir','/bin/deep')
    content = bytes(range(256))*5 + b'\0end\xff'
    payload.write_bytes(content)
    run(cardreader,image,'put','/bin/deep/test.bin',payload)
    run(cardreader,image,'get','/bin/deep/test.bin',exported)
    assert exported.read_bytes() == content
    assert b'/bin/deep/test.bin' in run(cardreader,image,'find','test.bin').stdout
    assert b'bytes=1285' in run(cardreader,image,'du','/bin').stdout
    snapshot = image.read_bytes()
    run(cardreader,image,'touch','/bin/deep/test.bin',ok=False)
    run(cardreader,image,'get','/missing',ok=False)
    run(cardreader,image,'cd','/missing',ok=False)
    assert image.read_bytes() == snapshot
    payload.write_bytes(b'x'*100000)
    run(cardreader,image,'set','/bin/deep/test.bin',payload,ok=False)
    assert image.read_bytes() == snapshot, 'failed resize must preserve contents'
    for size in (0,1,299,300,301,620,621,1280,62700,1):
        payload.write_bytes(bytes((i*7)&255 for i in range(size)))
        run(cardreader,image,'set','/bin/deep/test.bin',payload)
        run(cardreader,image,'get','/bin/deep/test.bin',exported)
        assert exported.read_bytes() == payload.read_bytes(), f'AFS cluster boundary {size}'
    run(cardreader,image,'rm','/bin')
    data = image.read_bytes()
    assert data[257:649] == bytes(392), 'recursive delete must free the complete chains'
    assert data[649:2185] == bytes(1536)
    # Directory traversal, quoted filenames, EOF persistence, root parent handling.
    run(cardreader,image,input=b'cd ..\nmkdir "space dir"\ncd "space dir"\ntouch "a b.txt"\n')
    assert b'a b.txt' in run(cardreader,image,'ls','/space dir').stdout
    boot = p/'boot.bin'; boot.write_bytes(bytes.fromhex('0000'))
    run(cardreader,image,'boot',boot)
    assert image.read_bytes()[2:4] == b'\0\0'
    # Read-only commands never rewrite the image.
    timestamp = image.stat().st_mtime_ns
    run(cardreader,image,'ls')
    assert image.stat().st_mtime_ns == timestamp
    run(cardreader,'--format',image,ok=False)
    corrupt = p/'corrupt.bin'
    for data in (bytes(65536), b'\xfe\x01', b'\xfe\x02'+bytes(65534)):
        corrupt.write_bytes(data); run(cardreader,corrupt,'ls',ok=False); assert corrupt.read_bytes() == data
    # Chains may not cycle, cross-link, reference out-of-range IDs, or leave orphan continuations.
    payload.write_bytes(b'a'*400)
    run(cardreader,image,'put','chain.bin',payload)
    valid = image.read_bytes()
    for edit in ((2185+20,1),(2185+20,255),(258,2)):
        bad=bytearray(valid);bad[edit[0]]=edit[1];corrupt.write_bytes(bad)
        run(cardreader,corrupt,'ls',ok=False); assert corrupt.read_bytes()==bad
    # A formatted AFS card is a regular MPME image readable by the CPU.
    assemble('load r1, 0\nread & 0x0100\nkill\n')
    output = run(emulator,'--boot',binary,'--insert-card','0x0100',image,'--novideo','--debug',input=b'ni\nni\nsh info\nexit\n').stdout
    assert b'R1=fe01' in output

print('Toolchain tests passed: 65536 headers, truncations, random bytes, assembly diagnostics, UALf, boot and AFS persistence/corruption')
