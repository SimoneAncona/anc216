#pragma once
#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/CodeGen/SelectionDAGTargetInfo.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

#define GET_REGINFO_ENUM
#include "ANC216GenRegisterInfo.inc"
#define GET_INSTRINFO_ENUM
#include "ANC216GenInstrInfo.inc"
#define GET_REGINFO_HEADER
#include "ANC216GenRegisterInfo.inc"
#define GET_INSTRINFO_HEADER
#include "ANC216GenInstrInfo.inc"
#define GET_SUBTARGETINFO_HEADER
#include "ANC216GenSubtargetInfo.inc"

namespace llvm
{
namespace ANC216ISD
{
enum { RET = ISD::BUILTIN_OP_END, CALL, PUSHARG, POPARGS, STACKARG };
}

class ANC216RegisterInfo : public ANC216GenRegisterInfo
{
public:
    ANC216RegisterInfo();
    const MCPhysReg *getCalleeSavedRegs(const MachineFunction *) const override;
    const uint32_t *getCallPreservedMask(const MachineFunction &, CallingConv::ID) const override;
    BitVector getReservedRegs(const MachineFunction &) const override;
    bool eliminateFrameIndex(MachineBasicBlock::iterator, int, unsigned,
                             RegScavenger * = nullptr) const override;
    Register getFrameRegister(const MachineFunction &) const override;
};

class ANC216Subtarget;
class ANC216InstrInfo : public ANC216GenInstrInfo
{
    ANC216RegisterInfo registers;
public:
    explicit ANC216InstrInfo(const ANC216Subtarget &);
    const ANC216RegisterInfo &getRegisterInfo() const { return registers; }
    void copyPhysReg(MachineBasicBlock &, MachineBasicBlock::iterator, const DebugLoc &,
                     Register, Register, bool, bool = false, bool = false) const override;
    void storeRegToStackSlot(MachineBasicBlock &, MachineBasicBlock::iterator,
        Register, bool, int, const TargetRegisterClass *, Register,
        MachineInstr::MIFlag = MachineInstr::NoFlags) const override;
    void loadRegFromStackSlot(MachineBasicBlock &, MachineBasicBlock::iterator,
        Register, int, const TargetRegisterClass *, Register, unsigned = 0,
        MachineInstr::MIFlag = MachineInstr::NoFlags) const override;
    unsigned insertBranch(MachineBasicBlock &, MachineBasicBlock *, MachineBasicBlock *,
        ArrayRef<MachineOperand>, const DebugLoc &, int * = nullptr) const override;
};

class ANC216FrameLowering : public TargetFrameLowering
{
    bool hasFPImpl(const MachineFunction &) const override { return true; }
public:
    ANC216FrameLowering() : TargetFrameLowering(StackGrowsUp, Align(1), 0, Align(1)) {}
    void emitPrologue(MachineFunction &, MachineBasicBlock &) const override;
    void emitEpilogue(MachineFunction &, MachineBasicBlock &) const override {}
};

class ANC216Subtarget;
class ANC216TargetLowering : public TargetLowering
{
public:
    ANC216TargetLowering(const TargetMachine &, const ANC216Subtarget &);
    SDValue LowerFormalArguments(SDValue, CallingConv::ID, bool,
        const SmallVectorImpl<ISD::InputArg> &, const SDLoc &, SelectionDAG &,
        SmallVectorImpl<SDValue> &) const override;
    SDValue LowerReturn(SDValue, CallingConv::ID, bool,
        const SmallVectorImpl<ISD::OutputArg> &, const SmallVectorImpl<SDValue> &,
        const SDLoc &, SelectionDAG &) const override;
    SDValue LowerCall(CallLoweringInfo &, SmallVectorImpl<SDValue> &) const override;
    const char *getTargetNodeName(unsigned) const override;
};

class ANC216Subtarget : public ANC216GenSubtargetInfo
{
    ANC216InstrInfo instructions;
    ANC216FrameLowering frames;
    ANC216TargetLowering lowering;
    SelectionDAGTargetInfo dagInfo;
public:
    ANC216Subtarget(const Triple &, StringRef, StringRef, const TargetMachine &);
    void ParseSubtargetFeatures(StringRef, StringRef, StringRef);
    const ANC216InstrInfo *getInstrInfo() const override { return &instructions; }
    const ANC216RegisterInfo *getRegisterInfo() const override { return &instructions.getRegisterInfo(); }
    const ANC216FrameLowering *getFrameLowering() const override { return &frames; }
    const ANC216TargetLowering *getTargetLowering() const override { return &lowering; }
    const SelectionDAGTargetInfo *getSelectionDAGInfo() const override { return &dagInfo; }
    void initLibcallLoweringInfo(LibcallLoweringInfo &) const override;
};

class ANC216TargetMachine : public CodeGenTargetMachineImpl
{
    std::unique_ptr<TargetLoweringObjectFile> objectLowering;
    ANC216Subtarget subtarget;
public:
    ANC216TargetMachine(const Target &, const Triple &, StringRef, StringRef,
        const TargetOptions &, std::optional<Reloc::Model>, std::optional<CodeModel::Model>,
        CodeGenOptLevel, bool);
    ~ANC216TargetMachine() override;
    const ANC216Subtarget *getSubtargetImpl(const Function &) const override { return &subtarget; }
    TargetLoweringObjectFile *getObjFileLowering() const override { return objectLowering.get(); }
    TargetPassConfig *createPassConfig(PassManagerBase &) override;
};

FunctionPass *createANC216ISel(ANC216TargetMachine &, CodeGenOptLevel);
void initializeANC216ISelLegacyPass(PassRegistry &);
class AsmPrinter;
AsmPrinter *createANC216AssemblyPrinter(TargetMachine &, std::unique_ptr<MCStreamer> &&);
}
