#include "ANC216Target.h"
#include "TargetInfo/ANC216TargetInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;
#define GET_REGINFO_MC_DESC
#include "ANC216GenRegisterInfo.inc"
#define GET_INSTRINFO_MC_DESC
#include "ANC216GenInstrInfo.inc"
#define GET_SUBTARGETINFO_MC_DESC
#include "ANC216GenSubtargetInfo.inc"

namespace
{
class ANC216AsmInfo : public MCAsmInfo
{
public:
    explicit ANC216AsmInfo(const MCTargetOptions &options) : MCAsmInfo(options)
    {
        CodePointerSize = CalleeSaveStackSlotSize = 2;
        IsLittleEndian = false;
        StackGrowsUp = true;
        CommentString = ";";
        SupportsDebugInformation = false;
        ExceptionsType = ExceptionHandling::None;
    }
};

// The assembly pass prints complete ANC216 instructions as raw text. An MC
// printer is supplied for the streamer factory, but cannot emit object files.
class ANC216InstPrinter : public MCInstPrinter
{
public:
    using MCInstPrinter::MCInstPrinter;
    std::pair<const char *, uint64_t> getMnemonic(const MCInst &) const override
    {
        return {"", 0};
    }
    void printInst(const MCInst *, uint64_t, StringRef,
                   const MCSubtargetInfo &, raw_ostream &) override
    {
        report_fatal_error("ANC216 uses its assembly-only printer");
    }
};
}

extern "C" void LLVMInitializeANC216TargetMC()
{
    auto &target = getTheANC216Target();
    TargetRegistry::RegisterMCAsmInfo(target,
        [](const MCRegisterInfo &, const Triple &, const MCTargetOptions &options) -> MCAsmInfo *
        { return new ANC216AsmInfo(options); });
    TargetRegistry::RegisterMCRegInfo(target, [](const Triple &) -> MCRegisterInfo *
        { auto *info = new MCRegisterInfo(); InitANC216MCRegisterInfo(info, ANC216::PC); return info; });
    TargetRegistry::RegisterMCInstrInfo(target, []() -> MCInstrInfo *
        { auto *info = new MCInstrInfo(); InitANC216MCInstrInfo(info); return info; });
    TargetRegistry::RegisterMCSubtargetInfo(target,
        [](const Triple &triple, StringRef cpu, StringRef features) -> MCSubtargetInfo *
        { return createANC216MCSubtargetInfoImpl(triple, cpu.empty() ? "generic" : cpu,
                                                cpu.empty() ? "generic" : cpu, features); });
    TargetRegistry::RegisterMCInstPrinter(target,
        [](const Triple &, unsigned, const MCAsmInfo &ai, const MCInstrInfo &ii,
           const MCRegisterInfo &ri) -> MCInstPrinter * { return new ANC216InstPrinter(ai, ii, ri); });
}
