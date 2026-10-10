# LLVM backend for ANC216

This project reuses Clang and LLVM to produce assembly for the existing ANC216
assembler. Backend sources live in `lib/Target/ANC216`; `clang/ANC216.h` describes
C types and target macros. LLVM's **main branch** is the development base.

The initial implementation is tested in system mode and supports 16-bit integer code: arithmetic,
comparisons, branches, locals, byte/word memory access, and direct function calls.
Software helpers implement 16-bit multiplication, division and remainder. It does
not yet provide complete ANSI C support, a C library, OS startup, object files,
variadic calls, indirect calls, or a complete 32-bit ABI.

## Build LLVM main with ANC216

Run these commands from the ANC216 repository root:

```sh
git clone --depth 1 --branch main https://github.com/llvm/llvm-project.git llvm/upstream
python llvm/integrate.py llvm/upstream
cmake -S llvm/upstream/llvm -B build/llvm-upstream -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_PROJECTS=clang \
    -DLLVM_TARGETS_TO_BUILD= \
    -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=ANC216 \
    -DLLVM_DEFAULT_TARGET_TRIPLE=anc216-unknown-none \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DLLVM_ENABLE_ZLIB=OFF \
    -DLLVM_ENABLE_ZSTD=OFF \
    -DLLVM_ENABLE_LIBXML2=OFF
cmake --build build/llvm-upstream --target llc clang llvm-tblgen --parallel 2
build/llvm-upstream/bin/llc --version
```

Skip cloning if `llvm/upstream` already exists. `integrate.py` copies the target
and patches LLVM's architecture triple/data layout and Clang target selection.
It checks its integration anchors before writing changes and can be run repeatedly.
Do not edit copied backend files in `upstream`: edit this directory and rerun the
integration command before rebuilding. The upstream checkout is Git-ignored.

Use headers, libraries and TableGen from the same build. Record the main commit
with `git -C llvm/upstream rev-parse HEAD` when diagnosing compatibility problems.
When updating LLVM main, first restore only the integration edits in that local
checkout, then update, reintegrate, and rebuild. Do not change assertion/build
settings while a build is running.

## Compile C to ANC216 assembly

```sh
build/llvm-upstream/bin/clang -target anc216-unknown-none \
    -std=c89 -ffreestanding -fno-builtin -O1 -S -emit-llvm \
    llvm/examples/demo.c -o build/demo.ll
build/llvm-upstream/bin/llc -march=anc216 -O0 \
    build/demo.ll -o build/demo.anc216
```

Clang's target uses 8-bit chars, 16-bit ints/pointers, 32-bit longs, big-endian
words, and 1-byte alignment. Generated function symbols have a leading `_`
so names such as `add` do not collide with assembler keywords. Use the matching
Clang target; replacing the layout of host-generated IR is not sufficient.
The first two argument words use R0/R1; further arguments use the upward-growing
stack, with caller cleanup. This supports ordinary fixed signatures with more
than two arguments; variadic functions remain unsupported.

For OS applications, C uses `int main(const char *args, unsigned length)`.
Import `runtime/crt0.anc216` before the generated module to provide the assembler's
`_code` entry. Startup forwards the OS's raw command-line pointer (R1) and byte
length (R2) to C, and exits using the return value. It does not create `argc/argv`.
Package with a UALf header at logical origin zero and appropriate OS permission
flags. User PC/SP/BP and pointers use logical IMEM offsets.

For direct IR experiments:

```sh
build/llvm-upstream/bin/llc -march=anc216 -O0 \
    llvm/examples/add.ll -o build/add.anc216
```

The example returns a value in R0. Assembly functions call it with
`phbp; call & _add; pobp`. See [ABI.md](ABI.md) for arguments and preserved registers.
Missing register-indirect memory instructions are handled by temporarily saving BP
and using the pointer as a new BP. In user mode this pointer is logical, including
pointers to stack locals; system mode uses physical addresses.

## Verify generated programs in the emulator

```sh
cmake -S . -B build -G Ninja -DANC216_WITH_SDL=OFF \
    -DANC216_LLVM_LLC="$PWD/build/llvm-upstream/bin/llc"
cmake --build build --parallel 2
ctest --test-dir build -R '^llvm_codegen$' --output-on-failure
```

The integration test compiles C, emits ANC216 assembly, assembles it, and executes
it with the real CPU implementation. It checks results plus SP/BP and R4..R6
preservation across calls. No LLVM dependency is added to the normal tool builds;
compiler tests are enabled only when `ANC216_LLVM_LLC` is supplied.

To run the demo after building the tools:

```sh
cp llvm/examples/start.anc216 build/start.anc216
build/assembler/assembler build/start.anc216 build/demo.bin
build/llvm_guest_runner build/demo.bin 42
```

This runner loads the flat image at IMEM zero and starts at `0x100`. The emulator
CLI's 256-byte boot ROM is a different loading path; larger compiler outputs need
an appropriate loader rather than being passed directly as boot ROM.

The original registration/TableGen smoke project can also be built separately:

```sh
cmake -S llvm -B build/llvm \
    -DLLVM_DIR="$PWD/build/llvm-upstream/lib/cmake/llvm" \
    -DANC216_LLVM_TABLEGEN="$PWD/build/llvm-upstream/bin/llvm-tblgen"
cmake --build build/llvm --parallel 2
ctest --test-dir build/llvm --output-on-failure
```

## Development references

- [Writing an LLVM backend](https://llvm.org/docs/WritingAnLLVMBackend.html)
- [TableGen reference](https://llvm.org/docs/TableGen/ProgRef.html)
- [MSP430 backend on main](https://github.com/llvm/llvm-project/tree/main/llvm/lib/Target/MSP430)
- [ANC216 executable semantics](../doc/IMPLEMENTATION.md)
- [Canonical instruction encoding](../common/encoding.hh)
# Building OS C applications

`os/build.py` discovers `os/programs/*.c` alongside the existing assembly
commands. Each C file is one executable and must define `main`. Build as usual
with `./build.sh --release`, or run:

```sh
python3 os/build.py --assembler build/production/assembler/assembler \
  --cardreader build/production/cardreader/cardreader --output build/production/os
```

The default compiler paths are `build/llvm-upstream/bin/clang` and `llc`;
override them with `--clang` and `--llc`. Generated IR, assembly and the import
wrapper are retained under the output directory's `c/<program>/` folder.
The assembler imports `os/libs/c.anc216` first, then the generated C assembly,
resolves their labels together and writes one `<program>.ual`, installed on the
generated disk as `/bin/<program>`. This is static linking; there is no separate
runtime executable. Use `--c-runtime` to select another assembly runtime.

C external function `abc` refers to assembly label `_abc`. The startup calls
`main(const char *args, unsigned length)` with the OS's raw command-line string
and byte length, then exits using its return value. Current backend limitations
still apply, including unsupported 32-bit arguments (`unsigned long`) and the
calling convention described in [ABI.md](ABI.md).
