"""Exercise asynchronous debugger commands through its real terminal input."""
import os
import pty
import re
import select
import signal
import subprocess
import sys
import tempfile
import time
from pathlib import Path

with tempfile.TemporaryDirectory() as directory:
    boot = Path(directory) / 'loop.bin'
    boot.write_bytes(bytes.fromhex('012d8022ff00'))  # INC R0; JMP ff00
    proc = subprocess.Popen([sys.argv[1], '--boot', str(boot), '--novideo', '--debug', '--uncapped'],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output = bytearray()

    def expect(fragment, *, pattern=False):
        deadline = time.monotonic() + 5
        target = re.compile(fragment.encode() if pattern else re.escape(fragment.encode()))
        while (match := target.search(output)) is None:
            remaining = deadline - time.monotonic()
            assert remaining > 0, (fragment, output.decode(errors='replace'))
            assert select.select([proc.stdout], [], [], remaining)[0], (fragment, output.decode(errors='replace'))
            data = os.read(proc.stdout.fileno(), 65536)
            assert data, output.decode(errors='replace')
            output.extend(data)
        del output[:match.end()]

    def command(text):
        proc.stdin.write((text + '\n').encode())
        proc.stdin.flush()

    try:
        expect('(anc216) ')
        command('devices')
        # Column padding changes when ROM address ranges need more space.
        expect(r'0xfffc[ \t]+Keyboard[ \t]+0x0301', pattern=True)
        command('b pc')
        expect('Breakpoint set at 0xff00')
        command('c')
        expect('breakpoint at 0xff00')
        command('c')
        expect('Paused after 2 instructions (breakpoint at 0xff00)')
        command('r')
        expect('R0=0001')
        command('s')
        expect('R0=0002')
        command('set l0 0xab')
        expect('Paused after 3 instructions')
        command('r')
        expect('R0=00ab')
        command('x pc-2 2')
        expect('0xff00  01 2d')
        command('set sr 256')
        expect('Error:')
        command('disable ff00')
        expect('Breakpoint disable at 0xff00')
        command('c')
        expect('Running; stop or Ctrl+C pauses.')
        proc.send_signal(signal.SIGINT)
        expect('Paused after')
        command('status')
        expect('Paused after')
        command('reset')
        expect('Paused after 0 instructions')
        command('bl')
        expect('[disabled] 0xff00')
        command('q')
        assert proc.wait(timeout=5) == 0
    finally:
        if proc.poll() is None:
            proc.kill()
            proc.wait()
    # A PTY enables the actual editor, rather than the plain pipe input path.
    master, slave = pty.openpty()
    proc = subprocess.Popen([sys.argv[1], '--boot', str(boot), '--novideo', '--debug'],
                            stdin=slave, stdout=slave, stderr=slave)
    os.close(slave)
    pending = bytearray()

    def terminal_expect(fragment):
        target = fragment.encode()
        deadline = time.monotonic() + 5
        while target not in pending:
            remaining = deadline - time.monotonic()
            assert remaining > 0, (fragment, pending.decode(errors='replace'))
            assert select.select([master], [], [], remaining)[0], (fragment, pending.decode(errors='replace'))
            pending.extend(os.read(master, 65536))
        del pending[:pending.index(target) + len(target)]

    try:
        terminal_expect('(anc216) ')
        os.write(master, b'reg\t\n')
        terminal_expect('R0=0000')
        os.write(master, b'\x1b[A\n')  # repeat regs from history
        terminal_expect('R0=0000')
        os.write(master, b'set r0 12x\x7f\n')  # backspace repairs the value
        terminal_expect('Paused after 0 instructions')
        os.write(master, b'r\n')
        terminal_expect('R0=000c')
        os.write(master, b'q\n')
        assert proc.wait(timeout=5) == 0
    finally:
        if proc.poll() is None:
            proc.kill()
            proc.wait()
        os.close(master)
print('Live breakpoints, editing, SIGINT pause and terminal history/completion passed')
