#include "ANC216Target.h"
#include "llvm/Analysis/LibcallLoweringInfo.h"
#include "TargetInfo/ANC216TargetInfo.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

using namespace llvm;
#define DEBUG_TYPE "anc216-target"
#define GET_REGINFO_TARGET_DESC
#include "ANC216GenRegisterInfo.inc"
#define GET_INSTRINFO_CTOR_DTOR
#include "ANC216GenInstrInfo.inc"
#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "ANC216GenSubtargetInfo.inc"

ANC216RegisterInfo::ANC216RegisterInfo() : ANC216GenRegisterInfo(ANC216::PC) {}
void ANC216Subtarget::initLibcallLoweringInfo(LibcallLoweringInfo &info) const
{
    info.setLibcallImpl(RTLIB::MUL_I16, RTLIB::impl___mulhi3);
    info.setLibcallImpl(RTLIB::SDIV_I16, RTLIB::impl___divhi3);
    info.setLibcallImpl(RTLIB::UDIV_I16, RTLIB::impl___udivhi3);
    info.setLibcallImpl(RTLIB::SREM_I16, RTLIB::impl___modhi3);
    info.setLibcallImpl(RTLIB::UREM_I16, RTLIB::impl___umodhi3);
}
const MCPhysReg *ANC216RegisterInfo::getCalleeSavedRegs(const MachineFunction *) const
{
    return CSR_ANC216_SaveList;
}
const uint32_t *ANC216RegisterInfo::getCallPreservedMask(const MachineFunction &, CallingConv::ID) const
{
    return CSR_ANC216_RegMask;
}
BitVector ANC216RegisterInfo::getReservedRegs(const MachineFunction &) const
{
    BitVector reserved(getNumRegs());
    for (auto reg : {ANC216::SP, ANC216::BP, ANC216::PC, ANC216::SR, ANC216::R7, ANC216::L7})
        reserved.set(reg);
    return reserved;
}
Register ANC216RegisterInfo::getFrameRegister(const MachineFunction &) const { return ANC216::BP; }
bool ANC216RegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator instruction,
    int spAdjustment, unsigned operand, RegScavenger *) const
{
    if (spAdjustment)
        report_fatal_error("ANC216: variable call frames are unsupported");
    auto &frame = instruction->getParent()->getParent()->getFrameInfo();
    const auto offset = frame.getObjectOffset(instruction->getOperand(operand).getIndex());
    if (offset < 0 || offset > 32767)
        report_fatal_error("ANC216: frame offset exceeds supported range");
    instruction->getOperand(operand).ChangeToImmediate(offset);
    return false;
}

ANC216InstrInfo::ANC216InstrInfo(const ANC216Subtarget &subtarget)
    : ANC216GenInstrInfo(subtarget, registers) {}
void ANC216InstrInfo::copyPhysReg(MachineBasicBlock &block, MachineBasicBlock::iterator at,
    const DebugLoc &location, Register dst, Register src, bool kill, bool, bool) const
{
    if (!ANC216::GPR16RegClass.contains(dst) || !ANC216::GPR16RegClass.contains(src))
        report_fatal_error("ANC216: unsupported physical register copy");
    auto instruction = BuildMI(block, at, location, get(ANC216::MOV16rr), dst)
        .addReg(src, getKillRegState(kill));
    // Copies must not create new flag dependencies during register allocation.
    instruction->getOperand(2).setIsDead(true);
}
void ANC216InstrInfo::storeRegToStackSlot(MachineBasicBlock &block, MachineBasicBlock::iterator at,
    Register src, bool kill, int index, const TargetRegisterClass *, Register,
    MachineInstr::MIFlag flags) const
{
    BuildMI(block, at, DebugLoc(), get(ANC216::STOREslot))
        .addReg(src, getKillRegState(kill)).addFrameIndex(index).setMIFlag(flags);
}
void ANC216InstrInfo::loadRegFromStackSlot(MachineBasicBlock &block, MachineBasicBlock::iterator at,
    Register dst, int index, const TargetRegisterClass *, Register, unsigned,
    MachineInstr::MIFlag flags) const
{
    BuildMI(block, at, DebugLoc(), get(ANC216::LOADslot), dst)
        .addFrameIndex(index).setMIFlag(flags);
}
unsigned ANC216InstrInfo::insertBranch(MachineBasicBlock &block, MachineBasicBlock *target,
    MachineBasicBlock *fallthrough, ArrayRef<MachineOperand> condition,
    const DebugLoc &location, int *bytes) const
{
    if (!condition.empty() || fallthrough)
        report_fatal_error("ANC216: unsupported branch insertion");
    BuildMI(block, block.end(), location, get(ANC216::JMP)).addMBB(target);
    if (bytes) *bytes = 4;
    return 1;
}
void ANC216FrameLowering::emitPrologue(MachineFunction &function, MachineBasicBlock &block) const
{
    auto &frame = function.getFrameInfo();
    if (frame.hasVarSizedObjects() || frame.getStackSize() > 32767)
        report_fatal_error("ANC216: variable or oversized stack frame");
    if (frame.getStackSize())
        BuildMI(block, block.begin(), DebugLoc(),
            function.getSubtarget().getInstrInfo()->get(ANC216::FRAME)).addImm(frame.getStackSize());
}

ANC216Subtarget::ANC216Subtarget(const Triple &triple, StringRef cpu, StringRef features,
    const TargetMachine &machine)
    : ANC216GenSubtargetInfo(triple, cpu.empty() ? "generic" : cpu,
          cpu.empty() ? "generic" : cpu, features), instructions(*this), lowering(machine, *this)
{
    ParseSubtargetFeatures(cpu.empty() ? "generic" : cpu, cpu.empty() ? "generic" : cpu, features);
}
ANC216TargetMachine::ANC216TargetMachine(const Target &target, const Triple &triple,
    StringRef cpu, StringRef features, const TargetOptions &options,
    std::optional<Reloc::Model> reloc, std::optional<CodeModel::Model> model,
    CodeGenOptLevel level, bool)
    : CodeGenTargetMachineImpl(target, triple, cpu, features, options,
          reloc.value_or(Reloc::Static), model.value_or(CodeModel::Small), level),
      objectLowering(std::make_unique<TargetLoweringObjectFileELF>()),
      subtarget(triple, cpu, features, *this)
{
    initAsmInfo();
}
ANC216TargetMachine::~ANC216TargetMachine() = default;
namespace
{
class ANC216Validate : public ModulePass
{
    static bool supported(Type *type)
    {
        return type->isVoidTy() || type->isPointerTy() ||
            (type->isIntegerTy() && type->getIntegerBitWidth() <= 16);
    }
public:
    static char ID;
    ANC216Validate() : ModulePass(ID) {}
    bool runOnModule(Module &module) override
    {
        if (!module.alias_empty() || !module.getModuleInlineAsm().empty())
            report_fatal_error("ANC216: aliases and module assembly are unsupported");
        for (const auto &global : module.globals())
            if (!global.isConstant() || !global.hasInitializer() ||
                !isa<ConstantDataArray>(global.getInitializer()) ||
                !cast<ConstantDataArray>(global.getInitializer())->getElementType()->isIntegerTy(8))
                report_fatal_error("ANC216: only constant byte-array globals are supported");
        for (const auto &function : module)
        {
            if (function.isIntrinsic()) continue;
            for (char c : function.getName())
                if (!(c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                      (c >= '0' && c <= '9')))
                    report_fatal_error("ANC216: function name cannot be represented by the assembler");
            if (!supported(function.getReturnType()) || function.isVarArg())
                report_fatal_error("ANC216: unsupported signature; use void/i16 returns and integer/pointer arguments");
            for (const auto &argument : function.args())
                if (!supported(argument.getType()))
                    report_fatal_error("ANC216: floating point, wide integers and aggregate arguments are unsupported");
            for (const auto &block : function)
                for (const auto &instruction : block)
                {
                    if (auto *intrinsic = dyn_cast<IntrinsicInst>(&instruction))
                        if (intrinsic->getIntrinsicID() == Intrinsic::lifetime_start ||
                            intrinsic->getIntrinsicID() == Intrinsic::lifetime_end)
                            continue;
                    if (!supported(instruction.getType()))
                        report_fatal_error("ANC216: floating point, wide integers and aggregate values are unsupported");
                    if (auto *call = dyn_cast<CallBase>(&instruction))
                        if (!call->getCalledFunction())
                            report_fatal_error("ANC216: indirect calls and inline assembly are unsupported");
                    for (const auto &operand : instruction.operands())
                    {
                        // Clang uses i32 constants for static allocation counts
                        // and some GEP indices even on a 16-bit target.
                        if ((isa<AllocaInst>(instruction) || isa<GetElementPtrInst>(instruction)) &&
                            isa<ConstantInt>(operand) &&
                            cast<ConstantInt>(operand)->getValue().isSignedIntN(16))
                            continue;
                        if (operand->getType()->isFloatingPointTy() ||
                            (operand->getType()->isIntegerTy() && operand->getType()->getIntegerBitWidth() > 16))
                            report_fatal_error("ANC216: floating point and wide integer operands are unsupported");
                    }
                }
        }
        return false;
    }
};
char ANC216Validate::ID;
RegisterPass<ANC216Validate> validateRegistration("anc216-validate", "Validate ANC216 IR subset", false, true);

class ANC216PassConfig : public TargetPassConfig
{
public:
    ANC216PassConfig(ANC216TargetMachine &machine, PassManagerBase &passes)
        : TargetPassConfig(machine, passes) {}
    void addIRPasses() override
    {
        addPass(new ANC216Validate());
        TargetPassConfig::addIRPasses();
    }
    bool addInstSelector() override
    {
        addPass(createANC216ISel(getTM<ANC216TargetMachine>(), getOptLevel()));
        return false;
    }
};
}
TargetPassConfig *ANC216TargetMachine::createPassConfig(PassManagerBase &passes)
{
    return new ANC216PassConfig(*this, passes);
}
extern "C" void LLVMInitializeANC216Target()
{
    RegisterTargetMachine<ANC216TargetMachine> registration(getTheANC216Target());
    initializeANC216ISelLegacyPass(*PassRegistry::getPassRegistry());
}
extern "C" void LLVMInitializeANC216AsmPrinter()
{
    TargetRegistry::RegisterAsmPrinter(getTheANC216Target(), createANC216AssemblyPrinter);
}
