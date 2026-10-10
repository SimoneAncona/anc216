#include "ANC216Target.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/IR/Function.h"
#include "llvm/InitializePasses.h"

using namespace llvm;

ANC216TargetLowering::ANC216TargetLowering(const TargetMachine &machine,
    const ANC216Subtarget &subtarget) : TargetLowering(machine, subtarget)
{
    addRegisterClass(MVT::i16, &ANC216::GPR16RegClass);
    computeRegisterProperties(subtarget.getRegisterInfo());
    setStackPointerRegisterToSaveRestore(ANC216::SP);
    setBooleanContents(ZeroOrOneBooleanContent);
    setOperationAction(ISD::BR_CC, MVT::i16, Legal);
    setOperationAction(ISD::BRCOND, MVT::Other, Expand);
    setOperationAction(ISD::SETCC, MVT::i16, Legal);
    setOperationAction(ISD::SELECT, MVT::i16, Legal);
    setOperationAction(ISD::SELECT_CC, MVT::i16, Expand);
    setOperationAction(ISD::MUL, MVT::i16, LibCall);
    setOperationAction(ISD::SDIV, MVT::i16, LibCall);
    setOperationAction(ISD::UDIV, MVT::i16, LibCall);
    setOperationAction(ISD::SREM, MVT::i16, LibCall);
    setOperationAction(ISD::UREM, MVT::i16, LibCall);
    setLibcallImpl(RTLIB::MUL_I16, RTLIB::impl___mulhi3);
    setLibcallImpl(RTLIB::SDIV_I16, RTLIB::impl___divhi3);
    setLibcallImpl(RTLIB::UDIV_I16, RTLIB::impl___udivhi3);
    setLibcallImpl(RTLIB::SREM_I16, RTLIB::impl___modhi3);
    setLibcallImpl(RTLIB::UREM_I16, RTLIB::impl___umodhi3);
    setOperationAction(ISD::SRA, MVT::i16, Legal);
    setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1, Expand);
    setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i8, Expand);
    setOperationAction(ISD::CTPOP, MVT::i16, Expand);
    setOperationAction(ISD::CTLZ, MVT::i16, Expand);
    setOperationAction(ISD::CTTZ, MVT::i16, Expand);
    setLoadExtAction(ISD::SEXTLOAD, MVT::i16, MVT::i8, Expand);
}

static void checkConvention(CallingConv::ID convention, bool variadic)
{
    if (convention != CallingConv::C || variadic)
        report_fatal_error("ANC216: only non-variadic C calls are supported");
}
SDValue ANC216TargetLowering::LowerFormalArguments(SDValue chain,
    CallingConv::ID convention, bool variadic, const SmallVectorImpl<ISD::InputArg> &inputs,
    const SDLoc &location, SelectionDAG &dag, SmallVectorImpl<SDValue> &values) const
{
    checkConvention(convention, variadic);
    const MCPhysReg registers[] = {ANC216::R0, ANC216::R1};
    auto &function = dag.getMachineFunction();
    for (unsigned i = 0; i < inputs.size(); ++i)
    {
        if (inputs[i].VT != MVT::i16 || inputs[i].Flags.isByVal())
            report_fatal_error("ANC216: unsupported argument type");
        if (i < 2) {
            Register reg = function.getRegInfo().createVirtualRegister(&ANC216::GPR16RegClass);
            function.getRegInfo().addLiveIn(registers[i], reg);
            values.push_back(dag.getCopyFromReg(chain, location, reg, MVT::i16));
        } else {
            const int64_t offset = -7 - 2 * int64_t(i - 2);
            if (offset < -32768) report_fatal_error("ANC216: too many stack arguments");
            auto argument = dag.getNode(ANC216ISD::STACKARG, location,
                dag.getVTList(MVT::i16, MVT::Other), chain,
                dag.getConstant(offset, location, MVT::i16));
            values.push_back(argument);
            chain = argument.getValue(1);
        }
    }
    return chain;
}
SDValue ANC216TargetLowering::LowerReturn(SDValue chain,
    CallingConv::ID convention, bool variadic, const SmallVectorImpl<ISD::OutputArg> &outputs,
    const SmallVectorImpl<SDValue> &values, const SDLoc &location, SelectionDAG &dag) const
{
    checkConvention(convention, variadic);
    if (outputs.size() > 1 || (!outputs.empty() && outputs[0].VT != MVT::i16))
        report_fatal_error("ANC216: only void/i16 returns are supported");
    SmallVector<SDValue, 4> operands;
    SDValue glue;
    if (!values.empty())
    {
        chain = dag.getCopyToReg(chain, location, ANC216::R0, values[0], SDValue());
        glue = chain.getValue(1);
    }
    operands.push_back(chain);
    if (glue)
    {
        operands.push_back(dag.getRegister(ANC216::R0, MVT::i16));
        operands.push_back(glue);
    }
    return dag.getNode(ANC216ISD::RET, location, MVT::Other, operands);
}
SDValue ANC216TargetLowering::LowerCall(CallLoweringInfo &call,
    SmallVectorImpl<SDValue> &values) const
{
    checkConvention(call.CallConv, call.IsVarArg);
    call.IsTailCall = false;
    auto &dag = call.DAG;
    auto chain = call.Chain;
    SDValue target = call.Callee;
    if (auto *global = dyn_cast<GlobalAddressSDNode>(target))
        target = dag.getTargetGlobalAddress(global->getGlobal(), call.DL, MVT::i16, global->getOffset());
    else if (auto *external = dyn_cast<ExternalSymbolSDNode>(target))
        target = dag.getTargetExternalSymbol(external->getSymbol(), MVT::i16);
    else
        report_fatal_error("ANC216: indirect calls are unsupported");
    const MCPhysReg registers[] = {ANC216::R0, ANC216::R1};
    SDValue glue;
    SmallVector<SDValue, 8> operands;
    for (const auto &argument : call.Outs)
        if (argument.VT != MVT::i16 || argument.Flags.isByVal())
            report_fatal_error("ANC216: unsupported call argument type");
    const unsigned stackBytes = call.Outs.size() > 2 ? 2 * (call.Outs.size() - 2) : 0;
    if (stackBytes > 32760) report_fatal_error("ANC216: too many stack arguments");
    // Upward stack: push last argument first so argument #3 is nearest BP.
    for (unsigned i = call.Outs.size(); i > 2; --i)
        chain = dag.getNode(ANC216ISD::PUSHARG, call.DL, MVT::Other,
            chain, call.OutVals[i - 1]);
    for (unsigned i = 0; i < std::min<unsigned>(call.Outs.size(), 2); ++i) {
        chain = dag.getCopyToReg(chain, call.DL, registers[i], call.OutVals[i], glue);
        glue = chain.getValue(1);
    }
    operands.push_back(chain);
    operands.push_back(target);
    for (unsigned i = 0; i < std::min<unsigned>(call.Outs.size(), 2); ++i)
        operands.push_back(dag.getRegister(registers[i], MVT::i16));
    operands.push_back(dag.getRegisterMask(dag.getSubtarget().getRegisterInfo()
        ->getCallPreservedMask(dag.getMachineFunction(), call.CallConv)));
    if (glue) operands.push_back(glue);
    chain = dag.getNode(ANC216ISD::CALL, call.DL,
        dag.getVTList(MVT::Other, MVT::Glue), operands);
    if (call.Ins.size() > 1 || (!call.Ins.empty() && call.Ins[0].VT != MVT::i16))
        report_fatal_error("ANC216: unsupported call return type");
    if (!call.Ins.empty())
    {
        auto result = dag.getCopyFromReg(chain, call.DL, ANC216::R0, MVT::i16, chain.getValue(1));
        values.push_back(result);
        chain = result.getValue(1);
    }
    if (stackBytes)
        chain = dag.getNode(ANC216ISD::POPARGS, call.DL, MVT::Other,
            chain, dag.getConstant(stackBytes, call.DL, MVT::i16));
    return chain;
}
const char *ANC216TargetLowering::getTargetNodeName(unsigned opcode) const
{
    switch (opcode)
    {
    case ANC216ISD::RET: return "ANC216ISD::RET";
    case ANC216ISD::CALL: return "ANC216ISD::CALL";
    case ANC216ISD::PUSHARG: return "ANC216ISD::PUSHARG";
    case ANC216ISD::POPARGS: return "ANC216ISD::POPARGS";
    case ANC216ISD::STACKARG: return "ANC216ISD::STACKARG";
    default: return nullptr;
    }
}

namespace
{
class ANC216ISel : public SelectionDAGISel
{
public:
    explicit ANC216ISel(ANC216TargetMachine &machine, CodeGenOptLevel level)
        : SelectionDAGISel(machine, level) {}
    void Select(SDNode *node) override
    {
        if (node->isMachineOpcode()) { node->setNodeId(-1); return; }
        SDLoc location(node);
        switch (node->getOpcode())
        {
        case ISD::GlobalAddress:
        {
            auto *address = cast<GlobalAddressSDNode>(node);
            CurDAG->SelectNodeTo(node, ANC216::MOV16ri, MVT::i16,
                CurDAG->getTargetGlobalAddress(address->getGlobal(), location,
                    MVT::i16, address->getOffset()));
            return;
        }
        case ANC216ISD::PUSHARG:
            CurDAG->SelectNodeTo(node, ANC216::PUSHARG, MVT::Other,
                node->getOperand(1), node->getOperand(0));
            return;
        case ANC216ISD::POPARGS:
            CurDAG->SelectNodeTo(node, ANC216::POPARGS, MVT::Other,
                CurDAG->getTargetConstant(cast<ConstantSDNode>(node->getOperand(1))->getZExtValue(), location, MVT::i16),
                node->getOperand(0));
            return;
        case ANC216ISD::STACKARG:
            CurDAG->SelectNodeTo(node, ANC216::LOADslot,
                CurDAG->getVTList(MVT::i16, MVT::Other), {
                CurDAG->getTargetConstant(cast<ConstantSDNode>(node->getOperand(1))->getSExtValue(), location, MVT::i16),
                node->getOperand(0)});
            return;
        case ISD::SRA:
            CurDAG->SelectNodeTo(node, ANC216::SRA16, MVT::i16,
                node->getOperand(0), node->getOperand(1));
            return;
        case ISD::SETCC:
            CurDAG->SelectNodeTo(node, ANC216::SETCC16, MVT::i16,
                node->getOperand(0), node->getOperand(1),
                CurDAG->getTargetConstant(cast<CondCodeSDNode>(node->getOperand(2))->get(),
                    location, MVT::i16));
            return;
        case ISD::SELECT:
            CurDAG->SelectNodeTo(node, ANC216::SELECT16, MVT::i16,
                node->getOperand(0), node->getOperand(1), node->getOperand(2));
            return;
        case ISD::FrameIndex:
        {
            int index = cast<FrameIndexSDNode>(node)->getIndex();
            CurDAG->SelectNodeTo(node, ANC216::FRAMEADDR, MVT::i16,
                CurDAG->getTargetFrameIndex(index, MVT::i16));
            return;
        }
        case ISD::BR:
            CurDAG->SelectNodeTo(node, ANC216::JMP, MVT::Other,
                node->getOperand(1), node->getOperand(0));
            return;
        case ISD::BR_CC:
            CurDAG->SelectNodeTo(node, ANC216::BRCC, MVT::Other, {
                node->getOperand(2), node->getOperand(3),
                CurDAG->getTargetConstant(cast<CondCodeSDNode>(node->getOperand(1))->get(),
                    location, MVT::i16), node->getOperand(4), node->getOperand(0)});
            return;
        case ISD::LOAD:
        {
            auto *load = cast<LoadSDNode>(node);
            unsigned opcode;
            if (load->getMemoryVT() == MVT::i16) opcode = ANC216::LOAD16;
            else if (load->getMemoryVT() == MVT::i8 &&
                     load->getExtensionType() != ISD::SEXTLOAD) opcode = ANC216::LOAD8Z;
            else report_fatal_error("ANC216: unsupported load width");
            auto *selected = CurDAG->getMachineNode(opcode, location,
                MVT::i16, MVT::Other, load->getBasePtr(), load->getChain());
            CurDAG->setNodeMemRefs(selected, {load->getMemOperand()});
            ReplaceNode(node, selected);
            return;
        }
        case ISD::STORE:
        {
            auto *store = cast<StoreSDNode>(node);
            unsigned opcode;
            if (store->getMemoryVT() == MVT::i16) opcode = ANC216::STORE16;
            else if (store->getMemoryVT() == MVT::i8) opcode = ANC216::STORE8;
            else report_fatal_error("ANC216: unsupported store width");
            auto *selected = CurDAG->getMachineNode(opcode, location, MVT::Other,
                store->getValue(), store->getBasePtr(), store->getChain());
            CurDAG->setNodeMemRefs(selected, {store->getMemOperand()});
            ReplaceNode(node, selected);
            return;
        }
        default: break;
        }
        SelectCode(node);
    }
#include "ANC216GenDAGISel.inc"
};
class ANC216ISelLegacy : public SelectionDAGISelLegacy
{
public:
    static char ID;
    ANC216ISelLegacy(ANC216TargetMachine &machine, CodeGenOptLevel level)
        : SelectionDAGISelLegacy(ID, std::make_unique<ANC216ISel>(machine, level)) {}
};
}
char ANC216ISelLegacy::ID;
INITIALIZE_PASS(ANC216ISelLegacy, "anc216-isel", "ANC216 instruction selection", false, false)
FunctionPass *llvm::createANC216ISel(ANC216TargetMachine &machine, CodeGenOptLevel level)
{
    return new ANC216ISelLegacy(machine, level);
}
