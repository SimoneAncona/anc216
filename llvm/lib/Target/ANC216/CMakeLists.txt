add_llvm_component_group(ANC216)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/ANC216Runtime.anc216 ANC216_RUNTIME_SOURCE)
configure_file(ANC216Runtime.inc.in ANC216Runtime.inc @ONLY)
set(LLVM_TARGET_DEFINITIONS ANC216.td)
tablegen(LLVM ANC216GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM ANC216GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM ANC216GenSubtargetInfo.inc -gen-subtarget)
tablegen(LLVM ANC216GenDAGISel.inc -gen-dag-isel)
add_public_tablegen_target(ANC216CommonTableGen)
add_llvm_target(ANC216CodeGen
    ANC216Target.cpp
    ANC216ISel.cpp
    ANC216AsmPrinter.cpp
    LINK_COMPONENTS ANC216Desc ANC216Info AsmPrinter CodeGen CodeGenTypes Core MC
        SelectionDAG Support Target TargetParser
    ADD_TO_COMPONENT ANC216)
add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
