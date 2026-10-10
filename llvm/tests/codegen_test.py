#!/usr/bin/env python3
"""Compile, assemble and execute code; check return values and ABI preservation."""
import argparse
import re
import subprocess
import tempfile
from pathlib import Path


def run(*args):
    subprocess.run([str(arg) for arg in args], check=True, capture_output=True, text=True)


CASES = {
    "string_literal": (101, "__attribute__((noinline)) int read_char(const volatile char *p) { return p[1]; }\n"
        "int main(void) { return read_char(\"hello\"); }"),
    "string_offset": (108, "__attribute__((noinline)) int read_char(const volatile char *p) { return p[0]; }\n"
        "int main(void) { return read_char(\"hello\"+2); }"),
    "type_alignment": (3, "int main(void) { return __alignof__(short)+__alignof__(int)+__alignof__(void *); }"),
    "struct_layout": (3, "struct S { char c; int x; }; int main(void) { return sizeof(struct S); }"),
    "mixed_locals": (42, "int main(void) { volatile unsigned char c=1; volatile int n=41; return c+n; }"),
    "three_arguments": (321, "__attribute__((noinline)) int sum(int a, int b, int c) { return a+10*b+100*c; }\n"
        "int main(void) { return sum(1, 2, 3); }"),
    "seven_arguments": (28, "__attribute__((noinline)) int sum(int a,int b,int c,int d,int e,int f,int g) { return a+b+c+d+e+f+g; }\n"
        "int main(void) { return sum(1,2,3,4,5,6,7); }"),
    "nested_stack_arguments": (84, "__attribute__((noinline)) int sum(int a,int b,int c,int d) { return a+b+c+d; }\n"
        "__attribute__((noinline)) int nested(int a,int b,int c,int d) { return sum(a,b,c,d)+sum(d,c,b,a); }\n"
        "int main(void) { return nested(10,20,5,7); }"),
    "stack_pointer_argument": (42, "__attribute__((noinline)) int read(int a,int b,volatile int *p) { return a+b+*p; }\n"
        "int main(void) { volatile int x=39; return read(1,2,&x); }"),
    "repeated_stack_calls": (60, "__attribute__((noinline)) int sum(int a,int b,int c) { return a+b+c; }\n"
        "int main(void) { volatile int n=10; int x=0; while(n) { x+=sum(1,2,3); --n; } return x; }"),
    "add": (15, "int main(void) { return 7 + 8; }"),
    "arguments": (15, "__attribute__((noinline)) int add(int a, int b) { return a+b; }\n"
        "int main(void) { return add(7, 8); }"),
    "nested": (33, "__attribute__((noinline)) int add(int a, int b) { return a+b; }\n"
        "__attribute__((noinline)) int twice(int a) { return add(a, a) + a; }\n"
        "int main(void) { return twice(11); }"),
    "locals": (42, "int main(void) { volatile int a=40; volatile int b=2; return a+b; }"),
    "unsigned": (1, "__attribute__((noinline)) int less(unsigned a, unsigned b) { return a<b; }\n"
        "int main(void) { return less(1, 65535u); }"),
    "unsigned_false": (0, "__attribute__((noinline)) int less(unsigned a, unsigned b) { return a<b; }\n"
        "int main(void) { return less(65535u, 1); }"),
    "signed": (1, "__attribute__((noinline)) int less(int a, int b) { return a<b; }\n"
        "int main(void) { return less(-32768, 32767); }"),
    "shift": (65532, "__attribute__((noinline)) int shift(int a, int b) { return a>>b; }\n"
        "int main(void) { return shift(-16, 2); }"),
    "byte": (255, "int main(void) { volatile unsigned char c=255; return c; }"),
    "loop": (15, "int main(void) { volatile int n=5; int total=0; while(n) { total+=n; --n; } return total; }"),
    "multiply": (65506, "__attribute__((noinline)) int product(int a, int b) { return a*b; }\n"
        "int main(void) { return product(-6, 5); }"),
    "divide": (65530, "__attribute__((noinline)) int divide(int a, int b) { return a/b; }\n"
        "int main(void) { return divide(-43, 7); }"),
    "remainder": (65535, "__attribute__((noinline)) int rem(int a, int b) { return a%b; }\n"
        "int main(void) { return rem(-43, 7); }"),
    "unsigned_divide": (1, "__attribute__((noinline)) unsigned divide(unsigned a, unsigned b) { return a/b; }\n"
        "int main(void) { return divide(65535u, 32768u); }"),
    "unsigned_remainder": (32767, "__attribute__((noinline)) unsigned rem(unsigned a, unsigned b) { return a%b; }\n"
        "int main(void) { return rem(65535u, 32768u); }"),
    "array_pointer": (42, "__attribute__((noinline)) int read(volatile int *p, int i) { return p[0]+p[i]; }\n"
        "int main(void) { volatile int a[80]; a[0]=40; a[79]=2; return read(a, 79); }"),
    "odd_stack_alignment": (1, "__attribute__((noinline)) int address(int *p) { return ((unsigned)p)&1; }\n"
        "int main(void) { int a; return address(&a); }"),

}
CASES["far_stack_argument"] = (68,
    "__attribute__((noinline)) int far(" +
    ",".join(f"int a{i}" for i in range(1, 66)) +
    ") { return a3+a65; }\nint main(void) { return far(" +
    ",".join(str(i) for i in range(1, 66)) + "); }")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--llc", type=Path, required=True)
    parser.add_argument("--clang", type=Path, required=True)
    parser.add_argument("--assembler", type=Path, required=True)
    parser.add_argument("--runner", type=Path, required=True)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="anc216-codegen-") as directory:
        folder = Path(directory)
        for name, (expected, source) in CASES.items():
            c_file = folder / f"{name}.c"
            ir_file = folder / f"{name}.ll"
            asm_file = folder / f"{name}.anc216"
            image = folder / f"{name}.bin"
            c_file.write_text(source)
            try:
                run(args.clang, "-target", "anc216-unknown-none", "-std=c89", "-ffreestanding",
                    "-fno-builtin", "-O1", "-S", "-emit-llvm", c_file, "-o", ir_file)
                run(args.llc, "-march=anc216", "-O0", ir_file, "-o", asm_file)
                generated = asm_file.read_text()
                assert "push byte 0" not in generated, "Calls must not insert alignment padding"
                assert not re.search(r"\bload\s+r[0-7]\s*,\s*0\s*(?:;[^\n]*)?$", generated,
                                     re.MULTILINE | re.IGNORECASE), "Use XOR to zero registers"
                asm_file.write_text("org 0x100\nphbp\ncall & _main\npobp\nkill\n" + generated)
                run(args.assembler, asm_file, image)
                run(args.runner, image, expected)
            except subprocess.CalledProcessError as error:
                print(f"FAIL {name}\n{error.stdout}\n{error.stderr}")
                if asm_file.exists(): print(asm_file.read_text())
                raise
            print(f"PASS {name}")


if __name__ == "__main__":
    main()
