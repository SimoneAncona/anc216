#pragma once

namespace llvm
{
    class Target;
    Target &getTheANC216Target();
}

extern "C" void LLVMInitializeANC216TargetInfo();
