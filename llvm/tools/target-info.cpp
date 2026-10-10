#include "TargetInfo/ANC216TargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"

int main()
{
    LLVMInitializeANC216TargetInfo();
    llvm::Triple triple("unknown-unknown-none");
    std::string error;
    const auto *target = llvm::TargetRegistry::lookupTarget("anc216", triple, error);
    if (!target)
    {
        llvm::errs() << error << '\n';
        return 1;
    }
    llvm::outs() << "Registered target: " << target->getName() << '\n'
                 << "Description: " << target->getShortDescription() << '\n'
                 << "Bootstrap only: instruction lowering is not implemented.\n";
    return 0;
}
