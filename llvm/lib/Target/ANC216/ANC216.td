include "llvm/Target/Target.td"
include "ANC216RegisterInfo.td"
include "ANC216InstrInfo.td"
defm : RemapAllTargetPseudoPointerOperands<GPR16>;

def ANC216InstrInfo : InstrInfo;
def ANC216 : Target {
    let InstructionSet = ANC216InstrInfo;
}

// Scheduling and feature descriptions will follow instruction lowering.
def : ProcessorModel<"generic", NoSchedModel, []>;
