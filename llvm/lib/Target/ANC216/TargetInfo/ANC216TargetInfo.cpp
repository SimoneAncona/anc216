#include "TargetInfo/ANC216TargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/TargetParser/Triple.h"

llvm::Target &llvm::getTheANC216Target()
{
    static llvm::Target target;
    return target;
}

extern "C" void LLVMInitializeANC216TargetInfo()
{
    llvm::RegisterTarget<llvm::Triple::anc216, false> registration(
        llvm::getTheANC216Target(), "anc216", "ANC216 16-bit processor", "ANC216");
}
