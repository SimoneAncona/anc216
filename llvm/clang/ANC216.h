#pragma once
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/MacroBuilder.h"

namespace clang::targets
{
class ANC216TargetInfo : public TargetInfo
{
public:
    ANC216TargetInfo(const llvm::Triple &triple, const TargetOptions &)
        : TargetInfo(triple)
    {
        BigEndian = true;
        TLSSupported = false;
        IntWidth = 16;
        IntAlign = ShortAlign = 8;
        LongWidth = 32;
        LongLongWidth = 64;
        LongAlign = LongLongAlign = 8;
        PointerWidth = 16;
        PointerAlign = 8;
        FloatWidth = 32;
        DoubleWidth = LongDoubleWidth = 64;
        HalfAlign = BFloat16Align = 8;
        FloatAlign = DoubleAlign = LongDoubleAlign = 8;
        SuitableAlign = 8;
        SizeType = UnsignedInt;
        PtrDiffType = IntPtrType = SignedInt;
        IntMaxType = SignedLongLong;
        SigAtomicType = SignedInt;
        resetDataLayout();
    }
    void getTargetDefines(const LangOptions &, MacroBuilder &builder) const override
    {
        builder.defineMacro("__ANC216__");
    }
    llvm::SmallVector<Builtin::InfosShard> getTargetBuiltins() const override { return {}; }
    ArrayRef<const char *> getGCCRegNames() const override
    {
        static const char *const names[] = {
            "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "sp", "bp", "pc", "sr"};
        return names;
    }
    ArrayRef<GCCRegAlias> getGCCRegAliases() const override { return {}; }
    bool validateAsmConstraint(const char *&, ConstraintInfo &) const override { return false; }
    std::string_view getClobbers() const override { return ""; }
    BuiltinVaListKind getBuiltinVaListKind() const override { return CharPtrBuiltinVaList; }
    bool allowsLargerPreferedTypeAlignment() const override { return false; }
};
}
