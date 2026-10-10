// Assembly-only backend: common/encoding.hh and the existing assembler encode
// the final instruction bytes. SR is modelled even for moves and loads.
def brtarget : Operand<OtherVT>;
def ANC216Ret : SDNode<"ANC216ISD::RET", SDTNone,
    [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def ANC216CallType : SDTypeProfile<0, 1, [SDTCisPtrTy<0>]>;
def ANC216Call : SDNode<"ANC216ISD::CALL", ANC216CallType,
    [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
class ANC216Inst<dag outs, dag ins, string assembly> : Instruction {
    let Namespace = "ANC216";
    let OutOperandList = outs;
    let InOperandList = ins;
    let AsmString = assembly;
    let Pattern = [];
    let Size = 2;
    let hasSideEffects = 0;
    let mayLoad = 0;
    let mayStore = 0;
}

// TRAN and ADD use destination, source operand order in ANC216 assembly.
let Defs = [SR] in {
    def MOV16rr : ANC216Inst<(outs GPR16:$dst), (ins GPR16:$src),
        "tran $dst, $src">;
    let Constraints = "$dst = $lhs" in
        def ADD16rr : ANC216Inst<(outs GPR16:$dst),
            (ins GPR16:$lhs, GPR16:$rhs), "add $dst, $rhs">;
    let Size = 4 in
        def MOV16ri : ANC216Inst<(outs GPR16:$dst), (ins i16imm:$value),
            "load $dst, $value">;
}

class Binary16<string name, SDNode operation> : ANC216Inst<
    (outs GPR16:$dst), (ins GPR16:$lhs, GPR16:$rhs),
    !strconcat(name, " $dst, $rhs")> {
    let Constraints = "$dst = $lhs";
    let Defs = [SR];
    let Pattern = [(set i16:$dst, (operation i16:$lhs, i16:$rhs))];
}
def SUB16rr : Binary16<"sub", sub>;
def AND16rr : Binary16<"and", and>;
def OR16rr : Binary16<"or", or>;
def XOR16rr : Binary16<"xor", xor>;
def SHL16rr : Binary16<"shl", shl>;
def SHR16rr : Binary16<"shr", srl>;

def : Pat<(add GPR16:$lhs, GPR16:$rhs), (ADD16rr GPR16:$lhs, GPR16:$rhs)>;
def : Pat<(i16 imm:$value), (MOV16ri imm:$value)>;

// Pseudos expand in the assembly printer. Memory operations temporarily use BP
// for general pointers; R7 is reserved for addressing/frame scratch work.
class ANC216Pseudo<dag outs, dag ins> : ANC216Inst<outs, ins, ""> {
    let isPseudo = 1;
    let Size = 0;
}
let Defs = [SR, R7] in {
    def SETCC16 : ANC216Pseudo<(outs GPR16:$dst),
        (ins GPR16:$lhs, GPR16:$rhs, i16imm:$condition)>;
    def SELECT16 : ANC216Pseudo<(outs GPR16:$dst),
        (ins GPR16:$condition, GPR16:$yes, GPR16:$no)>;
    def SRA16 : ANC216Pseudo<(outs GPR16:$dst), (ins GPR16:$value, GPR16:$amount)>;
}
let Defs = [SR], mayLoad = 1 in {
    def LOAD16 : ANC216Pseudo<(outs GPR16:$dst), (ins GPR16:$addr)>;
    def LOAD8Z : ANC216Pseudo<(outs GPR16:$dst), (ins GPR16:$addr)>;
    let Defs = [SR, R7] in
        def LOADslot : ANC216Pseudo<(outs GPR16:$dst), (ins i16imm:$offset)>;
}
let Defs = [SR], mayStore = 1 in {
    def STORE16 : ANC216Pseudo<(outs), (ins GPR16:$value, GPR16:$addr)>;
    def STORE8 : ANC216Pseudo<(outs), (ins GPR16:$value, GPR16:$addr)>;
    let Defs = [SR, R7] in
        def STOREslot : ANC216Pseudo<(outs), (ins GPR16:$value, i16imm:$offset)>;
}
def FRAMEADDR : ANC216Pseudo<(outs GPR16:$dst), (ins i16imm:$offset)> {
    let Uses = [BP];
    let Defs = [SR];
}
let hasSideEffects = 1, Uses = [SP], Defs = [SP, SR, R7] in
    def FRAME : ANC216Pseudo<(outs), (ins i16imm:$size)>;
let hasSideEffects = 1, mayStore = 1, Uses = [SP], Defs = [SP] in
    def PUSHARG : ANC216Pseudo<(outs), (ins GPR16:$value)>;
let hasSideEffects = 1, Uses = [SP], Defs = [SP, SR, R7] in
    def POPARGS : ANC216Pseudo<(outs), (ins i16imm:$size)>;

let isBranch = 1, isTerminator = 1 in {
    let isBarrier = 1 in
        def JMP : ANC216Inst<(outs), (ins brtarget:$target), "jmp $target">;
    // Compare and branch stay together: moves/loads also modify flags.
    let Defs = [SR, R7] in
        def BRCC : ANC216Pseudo<(outs),
            (ins GPR16:$lhs, GPR16:$rhs, i16imm:$condition, brtarget:$target)>;
}
let isCall = 1, hasSideEffects = 1, Uses = [SP, BP], Defs = [SP, BP, PC, SR, R7] in
    def CALL : ANC216Pseudo<(outs), (ins i16imm:$target)>;

// RET restores PC/SR and discards the CALL frame and locals. BP is managed
// by the caller. Return-value register uses belong in future call lowering.
let isReturn = 1, isTerminator = 1, isBarrier = 1,
    hasSideEffects = 1, mayLoad = 1, Uses = [BP], Defs = [PC, SP, SR] in
    def RET : ANC216Inst<(outs), (ins), "ret">;

def : Pat<(ANC216Ret), (RET)>;
def : Pat<(ANC216Call tglobaladdr:$target), (CALL tglobaladdr:$target)>;
def : Pat<(ANC216Call texternalsym:$target), (CALL texternalsym:$target)>;
