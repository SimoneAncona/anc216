// L0..L7 alias the low byte of R0..R7. A write to L preserves the
// corresponding high byte; lowering must account for that partial write.
def sub_lo : SubRegIndex<8, 0>;

class ANC216Reg<string name, bits<16> encoding> : Register<name> {
    let Namespace = "ANC216";
    let HWEncoding = encoding;
}

foreach index = 0-7 in {
    def L#index : ANC216Reg<!strconcat("l", !cast<string>(index)), index>;
    let SubRegIndices = [sub_lo] in {
        def R#index : RegisterWithSubRegs<
            !strconcat("r", !cast<string>(index)), [!cast<Register>("L"#index)]> {
            let Namespace = "ANC216";
            let HWEncoding = index;
        }
    }
}

// Special-register encodings here are internal identifiers, not operand bits.
def SP : ANC216Reg<"sp", 8>;
def BP : ANC216Reg<"bp", 9>;
def PC : ANC216Reg<"pc", 10>;
def SR : ANC216Reg<"sr", 11>;

def GPR16 : RegisterClass<"ANC216", [i16], 16,
    (add R0, R1, R2, R3, R4, R5, R6, R7)>;
def GPR8 : RegisterClass<"ANC216", [i8], 8,
    (add L0, L1, L2, L3, L4, L5, L6, L7)>;

def CSR_ANC216 : CalleeSavedRegs<(add R4, R5, R6)>;
