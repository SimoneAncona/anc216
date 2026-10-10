#!/usr/bin/env python3
"""Install the ANC216 target into a local LLVM main source checkout."""
import argparse
import shutil
from pathlib import Path


def replace_once(text, old, new):
    if new in text:
        return text
    if text.count(old) != 1:
        raise RuntimeError(f"LLVM main changed: expected exactly one {old!r}")
    return text.replace(old, new, 1)


def integrate(root):
    here = Path(__file__).resolve().parent
    edits = {}
    path = root / "llvm/include/llvm/TargetParser/Triple.h"
    edits[path] = replace_once(path.read_text(),
        "    msp430,      // MSP430: msp430",
        "    anc216,      // ANC216: anc216\n    msp430,      // MSP430: msp430")
    path = root / "llvm/lib/TargetParser/Triple.cpp"
    text = path.read_text()
    if 'return "anc216";' not in text:
        text = replace_once(text, '  case msp430:\n    return "msp430";',
            '  case anc216:\n    return "anc216";\n  case msp430:\n    return "msp430";')
        text = replace_once(text, '.Case("msp430", msp430)',
            '.Case("anc216", anc216)\n      .Case("msp430", msp430)')
        text = replace_once(text, '.Case("msp430", Triple::msp430)',
            '.Case("anc216", Triple::anc216)\n          .Case("msp430", Triple::msp430)')
        # ANC216 shares the 16-bit width and lack of wider variants with MSP430.
        # It differs in byte order and exception handling.
        text = text.replace('  case Triple::msp430:',
                            '  case Triple::anc216:\n  case Triple::msp430:')
        text = text.replace('  case llvm::Triple::msp430:',
                            '  case llvm::Triple::anc216:\n  case llvm::Triple::msp430:')
        start = text.index('bool Triple::isLittleEndian() const')
        end = text.index('\n}', start)
        text = text[:start] + text[start:end].replace('  case Triple::anc216:\n', '') + text[end:]
        start = text.index('ExceptionHandling Triple::getDefaultExceptionHandling() const')
        text = text[:start] + text[start:].replace('  case Triple::anc216:\n', '')
        start = text.index('Triple Triple::getLittleEndianArchVariant() const')
        text = text[:start] + text[start:].replace('  case Triple::UnknownArch:',
            '  case Triple::anc216:\n  case Triple::UnknownArch:', 1)
    edits[path] = text
    path = root / "llvm/lib/TargetParser/TargetDataLayout.cpp"
    layout = "E-p:16:8-i16:8-i32:8-i64:8-f32:8-f64:8-a:8-n16-S8"
    text = path.read_text()
    for old in ("E-p:16:16-i32:16-i64:16-f32:16-f64:16-a:8-n16-S8",
                "E-p:16:16-i32:16-i64:16-f32:16-f64:16-a:8-n16-S16"):
        text = text.replace(old, layout)
    edits[path] = replace_once(text, '  case Triple::msp430:\n',
        '  case Triple::anc216:\n'
        f'    return "{layout}";\n'
        '  case Triple::msp430:\n')
    path = root / "clang/lib/Basic/Targets.cpp"
    text = replace_once(path.read_text(), '#include "Targets/MSP430.h"',
        '#include "Targets/ANC216.h"\n#include "Targets/MSP430.h"')
    edits[path] = replace_once(text, '  case llvm::Triple::msp430:\n',
        '  case llvm::Triple::anc216:\n'
        '    return std::make_unique<ANC216TargetInfo>(Triple, Opts);\n'
        '  case llvm::Triple::msp430:\n')
    # Check every integration anchor before writing anything.
    for path, text in edits.items():
        if path.read_text() != text:
            path.write_text(text)
    # Preserve unchanged timestamps: Triple.h is included throughout LLVM.
    sources = here / "lib/Target/ANC216"
    destination = root / "llvm/lib/Target/ANC216"
    for source in sources.rglob("*"):
        if not source.is_file():
            continue
        target = destination / source.relative_to(sources)
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.exists() or target.read_bytes() != source.read_bytes():
            shutil.copyfile(source, target)
    source = here / "clang/ANC216.h"
    target = root / "clang/lib/Basic/Targets/ANC216.h"
    if not target.exists() or target.read_bytes() != source.read_bytes():
        shutil.copyfile(source, target)
    print(f"Installed ANC216 backend and Clang target into {root}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkout", type=Path)
    integrate(parser.parse_args().checkout.resolve())
