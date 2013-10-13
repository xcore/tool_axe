// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

// TODO call LLVMDisposeBuilder(), other cleanup.

#include "JIT.h"
#include "Config.h"

#if AXE_ENABLE_JIT

#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/BitReader.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Target.h"
#include "llvm-c/Transforms/Scalar.h"
#ifdef DEBUG_JIT
#include "llvm-c/Analysis.h"
#endif
#include "Instruction.h"
#include "InstructionOpcode.h"
#include "Core.h"
#include "InstructionBitcode.h"
#include "LLVMExtra.h"
#include "InstructionProperties.h"
#include "JITOptimize.h"
#include "JitGlobalMap.h"
#include "InstFunction.h"
#include "Array.h"
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <map>
#include <vector>

#define DEBUG_JIT false

using namespace axe;

struct JITFunctionInfo {
  explicit JITFunctionInfo(uint32_t a) :
    pc(a), value(0), func(0), isStub(false) {}
  JITFunctionInfo(uint32_t a, LLVMValueRef v, InstFunction_t f,
                  bool s) :
    pc(a), value(v), func(f), isStub(s) {}
  uint32_t pc;
  LLVMValueRef value;
  InstFunction_t func;
  std::set<JITFunctionInfo*> references;
  bool isStub;
};

struct JITCoreInfo {
  std::vector<uint32_t> unreachableFunctions;
  std::map<uint32_t, JITFunctionInfo*> functionMap;
  ~JITCoreInfo();
};

JITCoreInfo::~JITCoreInfo()
{
  for (auto &entry : functionMap) {
    delete entry.second;
  }
}

class axe::JITImpl {
  bool initialized;
  struct Functions {
    LLVMValueRef jitStubImpl;
    LLVMValueRef jitGetPc;
    LLVMValueRef jitUpdateExecutionFrequency;
    LLVMValueRef jitComputeAddress;
    LLVMValueRef jitCheckAddress;
    LLVMValueRef jitInvalidateByteCheck;
    LLVMValueRef jitInvalidateShortCheck;
    LLVMValueRef jitInvalidateWordCheck;
    LLVMValueRef jitInterpretOne;
    void init(LLVMModuleRef mod);
  };
  Functions functions;
  LLVMContextRef context;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMExecutionEngineRef executionEngine;
  LLVMTypeRef jitFunctionType;
  LLVMPassManagerRef FPM;

  std::map<const Core*,JITCoreInfo*> jitCoreMap;
  std::vector<LLVMValueRef> earlyReturnIncomingValues;
  std::vector<LLVMBasicBlockRef> earlyReturnIncomingBlocks;

  LLVMValueRef threadParam;
  LLVMValueRef ramSizeLog2Param;
  LLVMBasicBlockRef earlyReturnBB;
  LLVMBasicBlockRef interpretOneBB;
  LLVMBasicBlockRef endTraceBB;
  LLVMValueRef earlyReturnPhi;
  std::vector<LLVMValueRef> calls;

  void init();
  LLVMValueRef getCurrentFunction();
  void resetPerFunctionState();
  void reclaimUnreachableFunctions(JITCoreInfo &coreInfo);
  void reclaimUnreachableFunctions();
  void emitCondEarlyReturn(LLVMValueRef cond, LLVMValueRef retval);
  void checkReturnValue(LLVMValueRef call, InstructionProperties &properties);
  void emitCondBrToBlock(LLVMValueRef cond, LLVMBasicBlockRef trueBB);
  void ensureEarlyReturnBB(LLVMTypeRef phiType);
  LLVMBasicBlockRef appendBBToCurrentFunction(const char *name);
  LLVMValueRef emitCallToBeInlined(LLVMValueRef fn, LLVMValueRef *args,
                                   unsigned numArgs);
  JITCoreInfo *getJITCoreInfo(const Core &);
  JITCoreInfo *getOrCreateJITCoreInfo(const Core &);
  JITFunctionInfo *getJITFunctionOrStubImpl(JITCoreInfo &coreInfo, uint32_t pc);
  LLVMValueRef getJITFunctionOrStub(JITCoreInfo &coreIfno, uint32_t pc,
                                    JITFunctionInfo *caller);
  bool compileOneFragment(Core &core, JITCoreInfo &coreInfo, uint32_t pc,
                          bool &endOfBlock, uint32_t &nextPc);
  LLVMBasicBlockRef getOrCreateMemoryCheckBailoutBlock(unsigned index);
  void emitMemoryChecks(unsigned index,
                        std::queue<std::pair<uint32_t,MemoryCheck*>> &checks);
  LLVMValueRef getJitInvalidateFunction(unsigned size);
  void emitJumpToNextFragment(JITCoreInfo &coreInfo, uint32_t targetPc,
                              JITFunctionInfo *caller);
  bool emitJumpToNextFragment(InstructionOpcode opc, const Operands &operands,
                              JITCoreInfo &coreInfo, uint32_t nextPc,
                              JITFunctionInfo *caller);
  InstFunction_t getFunctionThunk(JITFunctionInfo &info);
public:
  JITImpl() : initialized(false) {}
  ~JITImpl();
  static JITImpl instance;
  static void initializeGlobalState();
  bool invalidate(Core &c, uint32_t pc);
  void compileBlock(Core &core, uint32_t pc);
};

JITImpl::~JITImpl()
{
  if (initialized) {
    LLVMDisposePassManager(FPM);
    LLVMDisposeBuilder(builder);
    LLVMDisposeExecutionEngine(executionEngine);
    LLVMContextDispose(context);
  }
  for (auto &entry : jitCoreMap) {
    delete entry.second;
  }
}

void JITImpl::Functions::init(LLVMModuleRef module)
{
  struct {
    const char *name;
    LLVMValueRef *ref;
  } initInfo[] = {
    { "jitStubImpl", &jitStubImpl },
    { "jitGetPc", &jitGetPc },
    { "jitUpdateExecutionFrequency", &jitUpdateExecutionFrequency },
    { "jitComputeAddress", &jitComputeAddress },
    { "jitCheckAddress", &jitCheckAddress },
    { "jitInvalidateByteCheck", &jitInvalidateByteCheck },
    { "jitInvalidateShortCheck", &jitInvalidateShortCheck },
    { "jitInvalidateWordCheck", &jitInvalidateWordCheck },
    { "jitInterpretOne", &jitInterpretOne },
  };
  for (unsigned i = 0; i < arraySize(initInfo); i++) {
    *initInfo[i].ref = LLVMGetNamedFunction(module, initInfo[i].name);
    assert(*initInfo[i].ref && "function not found in module");
  }
}

void JITImpl::initializeGlobalState()
{
  static bool initialized = false;
  if (initialized)
    return;
  LLVMLinkInJIT();
  LLVMInitializeNativeTarget();
  initialized = true;
}

static void addGlobalMappings(LLVMExecutionEngineRef ee, LLVMModuleRef module) {
  for (unsigned i = 0; i != jitFunctionMapSize; ++i) {
    const auto &entry = jitFunctionMap[i];
    LLVMValueRef val = LLVMGetNamedFunction(module, entry.first);
    if (!val)
      continue;
    LLVMAddGlobalMapping(ee, val, reinterpret_cast<void*>(entry.second));
  }
}

void JITImpl::init()
{
  if (initialized)
    return;
  JIT::initializeGlobalState();
  context = LLVMContextCreate();
  LLVMMemoryBufferRef memBuffer = LLVMCreateMemoryBufferWithMemoryRange(
    reinterpret_cast<const char*>(instructionBitcode), instructionBitcodeSize,
    "", 0);
  char *outMessage;
  if (LLVMParseBitcodeInContext(context, memBuffer, &module, &outMessage)) {
    std::cerr << "Error loading bitcode: " << outMessage << '\n';
    std::abort();
  }
  // TODO experiment with opt level.
  if (LLVMCreateJITCompilerForModule(&executionEngine, module, 1,
                                      &outMessage)) {
    std::cerr << "Error creating JIT compiler: " << outMessage << '\n';
    std::abort();
  }
  LLVMDisableSymbolSearching(executionEngine, 1);
  addGlobalMappings(executionEngine, module);
  builder = LLVMCreateBuilderInContext(context);
  LLVMValueRef callee = LLVMGetNamedFunction(module, "jitInstructionTemplate");
  assert(callee && "jitInstructionTemplate() not found in module");
  jitFunctionType = LLVMGetElementType(LLVMTypeOf(callee));
  functions.init(module);
  FPM = LLVMCreateFunctionPassManagerForModule(module);
  LLVMAddTargetData(LLVMGetExecutionEngineTargetData(executionEngine), FPM);
  LLVMAddTypeBasedAliasAnalysisPass(FPM);
  LLVMAddBasicAliasAnalysisPass(FPM);
  LLVMAddJumpThreadingPass(FPM);
  LLVMAddGVNPass(FPM);
  LLVMAddJumpThreadingPass(FPM);
  LLVMAddCFGSimplificationPass(FPM);
  LLVMAddDeadStoreEliminationPass(FPM);
  LLVMAddInstructionCombiningPass(FPM);
  LLVMInitializeFunctionPassManager(FPM);
  if (DEBUG_JIT) {
    LLVMExtraRegisterJitDisassembler(executionEngine, LLVMGetTarget(module));
  }
  initialized = true;
}

LLVMValueRef JITImpl::getCurrentFunction()
{
  return LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
}

void JITImpl::resetPerFunctionState()
{
  threadParam = 0;
  ramSizeLog2Param = 0;
  earlyReturnBB = 0;
  interpretOneBB = 0;
  endTraceBB = 0;
  earlyReturnIncomingValues.clear();
  earlyReturnIncomingBlocks.clear();
  calls.clear();
}

static bool
getInstruction(Core &core, uint32_t address, InstructionOpcode &opc,
               Operands &operands)
{
  if (!core.isValidRamPc(core.toRamPc(address)))
    return false;
  instructionDecode(core, address, opc, operands);
  return true;
}

void JITImpl::reclaimUnreachableFunctions(JITCoreInfo &coreInfo)
{
  std::vector<uint32_t> &unreachableFunctions = coreInfo.unreachableFunctions;
  std::map<uint32_t,JITFunctionInfo*> &functionMap = coreInfo.functionMap;
  for (uint32_t addr : unreachableFunctions) {
    auto entry = functionMap.find(addr);
    if (entry == functionMap.end())
      continue;
    LLVMValueRef value = entry->second->value;
    LLVMFreeMachineCodeForFunction(executionEngine, value);
    LLVMReplaceAllUsesWith(value, LLVMGetUndef(LLVMTypeOf(value)));
    LLVMDeleteFunction(value);
    delete entry->second;
    functionMap.erase(entry);
  }
  unreachableFunctions.clear();
}

void JITImpl::reclaimUnreachableFunctions()
{
  for (auto &entry : jitCoreMap) {
    reclaimUnreachableFunctions(*entry.second);
  }
}

static bool mayReturnEarly(InstructionProperties &properties)
{
  return properties.mayYield() || properties.mayEndTrace() ||
         properties.mayDeschedule();
}

void JITImpl::emitCondEarlyReturn(LLVMValueRef cond, LLVMValueRef retval)
{
  ensureEarlyReturnBB(LLVMGetReturnType((jitFunctionType)));
  earlyReturnIncomingValues.push_back(retval);
  earlyReturnIncomingBlocks.push_back(LLVMGetInsertBlock(builder));
  emitCondBrToBlock(cond, earlyReturnBB);
}

void
JITImpl::checkReturnValue(LLVMValueRef call, InstructionProperties &properties)
{
  if (!mayReturnEarly(properties))
    return;
  LLVMValueRef cmp =
    LLVMBuildICmp(builder, LLVMIntNE, call,
                  LLVMConstInt(LLVMTypeOf(call), 0,
                               static_cast<int>(InstReturn::CONTINUE)), "");
  emitCondEarlyReturn(cmp, call);
}

void JITImpl::ensureEarlyReturnBB(LLVMTypeRef phiType)
{
  if (earlyReturnBB)
    return;
  // Save off current insert point.
  LLVMBasicBlockRef savedBB = LLVMGetInsertBlock(builder);
  LLVMValueRef f = LLVMGetBasicBlockParent(savedBB);
  earlyReturnBB = LLVMAppendBasicBlockInContext(context, f, "early_return");
  LLVMPositionBuilderAtEnd(builder, earlyReturnBB);
  // Create phi (incoming values will be filled in later).
  earlyReturnPhi = LLVMBuildPhi(builder, phiType, "");
  LLVMBuildRet(builder, earlyReturnPhi);
  // Restore insert point.
  LLVMPositionBuilderAtEnd(builder, savedBB);
}

LLVMValueRef JITImpl::
emitCallToBeInlined(LLVMValueRef fn, LLVMValueRef *args, unsigned numArgs)
{
  LLVMValueRef call = LLVMBuildCall(builder, fn, args, numArgs, "");
  calls.push_back(call);
  return call;
}

void JITImpl::compileBlock(Core &core, uint32_t pc)
{
  init();
  reclaimUnreachableFunctions();
  bool endOfBlock;
  uint32_t nextPc;
  JITCoreInfo &coreInfo = *getOrCreateJITCoreInfo(core);
  do {
    compileOneFragment(core, coreInfo, pc, endOfBlock, nextPc);
    pc = nextPc;
  } while (!endOfBlock);
}

static bool
getSuccessors(InstructionOpcode opc, const Operands &operands,
              uint32_t nextPc, std::set<uint32_t> &successors)
{
  switch (opc) {
  default:
    return false;
  case BRFT_ru6:
  case BRFT_lru6:
  case BRBT_ru6:
  case BRBT_lru6:
  case BRFF_ru6:
  case BRFF_lru6:
  case BRBF_ru6:
  case BRBF_lru6:
    successors.insert(nextPc);
    successors.insert(operands.ops[1]);
    return true;
  case BRFU_u6:
  case BRFU_lu6:
  case BRBU_u6:
  case BRBU_lu6:
    successors.insert(operands.ops[0]);
    return true;
  case BLRF_u10:
  case BLRF_lu10:
  case BLRB_u10:
  case BLRB_lu10:
    successors.insert(operands.ops[0]);
    return true;
  case LDAPB_u10:
  case LDAPB_lu10:
  case LDAPF_u10:
  case LDAPF_lu10:
    successors.insert(nextPc);
    return true;
  }
}

JITCoreInfo *JITImpl::getJITCoreInfo(const Core &c)
{
  auto it = jitCoreMap.find(&c);
  if (it != jitCoreMap.end())
    return it->second;
  return 0;
}

JITCoreInfo *JITImpl::getOrCreateJITCoreInfo(const Core &c)
{
  if (JITCoreInfo *info = getJITCoreInfo(c))
    return info;
  JITCoreInfo *info = new JITCoreInfo;
  jitCoreMap.insert(std::make_pair(&c, info));
  return info;
}

JITFunctionInfo *JITImpl::
getJITFunctionOrStubImpl(JITCoreInfo &coreInfo, uint32_t pc)
{
  JITFunctionInfo *&info = coreInfo.functionMap[pc];
  if (info)
    return info;
  LLVMBasicBlockRef savedInsertPoint = LLVMGetInsertBlock(builder);
  LLVMValueRef f = LLVMAddFunction(module, "", jitFunctionType);
  LLVMSetFunctionCallConv(f, LLVMFastCallConv);
  LLVMBasicBlockRef entryBB =
    LLVMAppendBasicBlockInContext(context, f, "entry");
  LLVMPositionBuilderAtEnd(builder, entryBB);
  LLVMValueRef args[] = {
    LLVMGetParam(f, 0)
  };
  LLVMValueRef call =
    LLVMBuildCall(builder, functions.jitStubImpl, args, 1, "");
  LLVMBuildRet(builder, call);
  if (DEBUG_JIT) {
    LLVMDumpValue(f);
    LLVMVerifyFunction(f, LLVMAbortProcessAction);
  }
  InstFunction_t code =
    reinterpret_cast<InstFunction_t>(
     LLVMGetPointerToGlobal(executionEngine, f));
  info = new JITFunctionInfo(pc, f, code, true);
  LLVMPositionBuilderAtEnd(builder, savedInsertPoint);
  return info;
}

LLVMValueRef JITImpl::
getJITFunctionOrStub(JITCoreInfo &coreInfo, uint32_t pc,
                     JITFunctionInfo *caller)
{
  JITFunctionInfo *info = getJITFunctionOrStubImpl(coreInfo, pc);
  info->references.insert(caller);
  return info->value;
}

LLVMBasicBlockRef JITImpl::appendBBToCurrentFunction(const char *name)
{
  LLVMBasicBlockRef currentBB = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(currentBB);
  return LLVMAppendBasicBlockInContext(context, function, name);
}

void JITImpl::
emitJumpToNextFragment(JITCoreInfo &coreInfo, uint32_t targetPc,
                       JITFunctionInfo *caller)
{
  LLVMValueRef next = getJITFunctionOrStub(coreInfo, targetPc, caller);
  LLVMValueRef args[] = {
    threadParam
  };
  LLVMValueRef call = LLVMBuildCall(builder, next, args, 1, "");
  LLVMSetTailCall(call, true);
  LLVMSetInstructionCallConv(call, LLVMFastCallConv);
  LLVMBuildRet(builder, call);
}

bool JITImpl::
emitJumpToNextFragment(InstructionOpcode opc, const Operands &operands,
                       JITCoreInfo &coreInfo, uint32_t nextPc,
                       JITFunctionInfo *caller)
{
  std::set<uint32_t> successors;
  if (!getSuccessors(opc, operands, nextPc, successors))
    return false;
  unsigned numSuccessors = successors.size();
  if (numSuccessors == 0)
    return false;
  std::set<uint32_t>::iterator it = successors.begin();
  ++it;
  if (it != successors.end()) {
    LLVMValueRef args[] = {
      threadParam
    };
    LLVMValueRef nextPc = emitCallToBeInlined(functions.jitGetPc, args, 1);
    for (;it != successors.end(); ++it) {
      LLVMValueRef cmp =
        LLVMBuildICmp(builder, LLVMIntEQ, nextPc,
                      LLVMConstInt(LLVMTypeOf(nextPc), *it, false), "");
      LLVMBasicBlockRef trueBB = appendBBToCurrentFunction("");
      LLVMBasicBlockRef afterBB = appendBBToCurrentFunction("");
      LLVMBuildCondBr(builder, cmp, trueBB, afterBB);
      LLVMPositionBuilderAtEnd(builder, trueBB);
      emitJumpToNextFragment(coreInfo, *it, caller);
      LLVMPositionBuilderAtEnd(builder, afterBB);
    }
  }
  emitJumpToNextFragment(coreInfo, *successors.begin(), caller);
  return true;
}

static void deleteFunctionBody(LLVMValueRef f)
{
  LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(f);
  while (bb) {
    LLVMDeleteBasicBlock(bb);
    bb = LLVMGetFirstBasicBlock(f);
  }
}

static bool
getFragmentToCompile(Core &core, uint32_t startAddress,
                     std::vector<InstructionOpcode> &opcode,
                     std::vector<Operands> &operands, 
                     bool &endOfBlock, uint32_t &nextAddress)
{
  uint32_t address = startAddress;

  opcode.clear();
  operands.clear();
  endOfBlock = false;
  nextAddress = address;

  InstructionOpcode opc;
  Operands ops;
  InstructionProperties *properties;
  do {
    if (!getInstruction(core, address, opc, ops)) {
      endOfBlock = true;
      break;
    }
    instructionTransform(opc, ops, core, address);
    properties = &instructionProperties[opc];
    nextAddress = address + properties->size;
    if (properties->mayBranch())
      endOfBlock = true;
    if (!properties->function)
      break;
    opcode.push_back(opc);
    operands.push_back(ops);
    address = nextAddress;
  } while (!properties->mayBranch());
  return !opcode.empty();
}

/// Try and compile a fragment starting at the specified address. Returns
/// true if successful setting \a nextAddress to the first instruction after
/// the fragment. If unsuccessful returns false and sets \a nextAddress to the
/// address after the current function. \a endOfBlock is set to true if the
/// next address is in a new basic block.
bool JITImpl::
compileOneFragment(Core &core, JITCoreInfo &coreInfo, uint32_t startPc,
                   bool &endOfBlock, uint32_t &pcAfterFragment)
{
  assert(initialized);
  resetPerFunctionState();

  auto infoIt = coreInfo.functionMap.find(startPc);
  JITFunctionInfo *info =
    (infoIt == coreInfo.functionMap.end()) ? 0 : infoIt->second;
  if (info && !info->isStub) {
    endOfBlock = true;
    return false;
  }

  std::vector<InstructionOpcode> opcode;
  std::vector<Operands> operands;
  if (!getFragmentToCompile(core, core.fromRamPc(startPc), opcode, operands,
                            endOfBlock, pcAfterFragment)) {
    pcAfterFragment = core.toRamPc(pcAfterFragment);
    return false;
  }
  std::queue<std::pair<uint32_t,MemoryCheck*>> checks;
  placeMemoryChecks(opcode, operands, checks);

  LLVMValueRef f;
  if (info) {
    f = info->value;
    info->func = 0;
    info->isStub = false;
    deleteFunctionBody(f);
  } else {
    info = new JITFunctionInfo(startPc);
    coreInfo.functionMap.insert(std::make_pair(startPc, info));
    // Create function to contain the code we are about to add.
    info->value = f = LLVMAddFunction(module, "", jitFunctionType);
    LLVMSetFunctionCallConv(f, LLVMFastCallConv);
  }
  threadParam = LLVMGetParam(f, 0);
  LLVMValueRef ramBase =
    LLVMConstInt(LLVMInt32TypeInContext(context), core.getRamBase(), false);
  ramSizeLog2Param =
    LLVMConstInt(LLVMInt32TypeInContext(context), core.getRamSizeLog2(), false);
  LLVMBasicBlockRef entryBB =
    LLVMAppendBasicBlockInContext(context, f, "entry");
  LLVMPositionBuilderAtEnd(builder, entryBB);
  uint32_t pc = startPc;
  bool needsReturn = true;
  for (unsigned i = 0, e = opcode.size(); i != e; ++i) {
    InstructionOpcode opc = opcode[i];
    const Operands &ops = operands[i];
    InstructionProperties *properties = &instructionProperties[opc];
    uint32_t nextPc = pc + properties->size / 2;
    emitMemoryChecks(i, checks);

    // Lookup function to call.
    LLVMValueRef callee = LLVMGetNamedFunction(module, properties->function);
    assert(callee && "Function for instruction not found in module");
    LLVMTypeRef calleeType = LLVMGetElementType(LLVMTypeOf(callee));
    const unsigned fixedArgs = 4;
    const unsigned maxOperands = 6;
    unsigned numArgs = properties->getNumExplicitOperands() + fixedArgs;
    assert(LLVMCountParamTypes(calleeType) == numArgs);
    LLVMTypeRef paramTypes[fixedArgs + maxOperands];
    assert(numArgs <= (fixedArgs + maxOperands));
    LLVMGetParamTypes(calleeType, paramTypes);
    // Build call.
    LLVMValueRef args[fixedArgs + maxOperands];
    args[0] = threadParam;
    args[1] = LLVMConstInt(paramTypes[1], nextPc, false);
    args[2] = ramBase;
    args[3] = ramSizeLog2Param;
    for (unsigned i = fixedArgs; i < numArgs; i++) {
      uint32_t value =
      properties->getNumExplicitOperands() <= 3 ? ops.ops[i - fixedArgs] :
      ops.lops[i - fixedArgs];
      args[i] = LLVMConstInt(paramTypes[i], value, false);
    }
    LLVMValueRef call = emitCallToBeInlined(callee, args, numArgs);
    checkReturnValue(call, *properties);
    if (properties->mayBranch() && properties->function &&
        emitJumpToNextFragment(opc, ops, coreInfo, nextPc, info)) {
      needsReturn = false;
    }
    pc = nextPc;
  }
  assert(checks.empty() && "Not all checks emitted");
  if (needsReturn) {
    LLVMValueRef args[] = {
      threadParam
    };
    emitCallToBeInlined(functions.jitUpdateExecutionFrequency, args, 1);
    // Build return.
    LLVMBuildRet(builder,
                 LLVMConstInt(LLVMGetReturnType(jitFunctionType),
                              static_cast<int>(InstReturn::CONTINUE), 0));
  }
  // Add incoming phi values.
  if (earlyReturnBB) {
    LLVMAddIncoming(earlyReturnPhi, &earlyReturnIncomingValues[0],
                    &earlyReturnIncomingBlocks[0],
                    earlyReturnIncomingValues.size());
  }
  if (DEBUG_JIT) {
    LLVMDumpValue(f);
    LLVMVerifyFunction(f, LLVMAbortProcessAction);
  }
  // Optimize.
  for (LLVMValueRef call : calls) {
    LLVMExtraInlineFunction(call);
  }
  LLVMRunFunctionPassManager(FPM, f);
  if (DEBUG_JIT) {
    LLVMDumpValue(f);
  }
  // Compile.
  InstFunction_t compiledFunction =
    reinterpret_cast<InstFunction_t>(
      LLVMRecompileAndRelinkFunction(executionEngine, f));
  info->isStub = false;
  info->func = compiledFunction;
  core.setOpcode(startPc, getFunctionThunk(*info), (pc - startPc) * 2);
  return true;
}

void JITImpl::emitCondBrToBlock(LLVMValueRef cond, LLVMBasicBlockRef trueBB)
{
  LLVMBasicBlockRef afterBB =
    LLVMAppendBasicBlockInContext(context, getCurrentFunction(), "");
  LLVMBuildCondBr(builder, cond, trueBB, afterBB);
  LLVMPositionBuilderAtEnd(builder, afterBB);
}

LLVMBasicBlockRef JITImpl::getOrCreateMemoryCheckBailoutBlock(unsigned index)
{
  if (index == 0) {
    if (interpretOneBB) {
      return interpretOneBB;
    }
  } else if (endTraceBB) {
    return endTraceBB;
  }
  LLVMBasicBlockRef savedInsertPoint = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef bailoutBB =
    LLVMAppendBasicBlockInContext(context, getCurrentFunction(), "");
  LLVMPositionBuilderAtEnd(builder, bailoutBB);
  if (index == 0) {
    LLVMValueRef args[] = {
      threadParam
    };
    LLVMValueRef call = emitCallToBeInlined(functions.jitInterpretOne, args, 1);
    LLVMBuildRet(builder, call);
    interpretOneBB = bailoutBB;
  } else {
    ensureEarlyReturnBB(LLVMGetReturnType(jitFunctionType));
    earlyReturnIncomingValues.push_back(
      LLVMConstInt(LLVMGetReturnType(jitFunctionType),
                   static_cast<int>(InstReturn::END_TRACE), false));
    earlyReturnIncomingBlocks.push_back(LLVMGetInsertBlock(builder));
    LLVMBuildBr(builder, earlyReturnBB);
    endTraceBB = bailoutBB;
  }
  LLVMPositionBuilderAtEnd(builder, savedInsertPoint);
  return bailoutBB;
}

void JITImpl::
emitMemoryChecks(unsigned index,
                 std::queue<std::pair<uint32_t,MemoryCheck*>> &checks)
{
  while (!checks.empty() && checks.front().first == index) {
    MemoryCheck *check = checks.front().second;
    checks.pop();
    LLVMBasicBlockRef bailoutBB = getOrCreateMemoryCheckBailoutBlock(index);
    // Compute address.
    LLVMValueRef address;
    {
      LLVMTypeRef paramTypes[5];
      LLVMGetParamTypes(LLVMGetElementType(LLVMTypeOf(functions.jitComputeAddress)),
                        paramTypes);
      LLVMValueRef args[] = {
        threadParam,
        LLVMConstInt(paramTypes[1], check->getBaseReg(), false),
        LLVMConstInt(paramTypes[2], check->getScale(), false),
        LLVMConstInt(paramTypes[3], check->getOffsetReg(), false),
        LLVMConstInt(paramTypes[4], check->getOffsetImm(), false)
      };
      address = emitCallToBeInlined(functions.jitComputeAddress, args, 5);
    }
    // Check alignment.
    if (check->getFlags() & MemoryCheck::CheckAlignment &&
        check->getSize() > 1) {
      LLVMValueRef rem =
        LLVMBuildURem(
          builder, address,
          LLVMConstInt(LLVMTypeOf(address), check->getSize(), false), "");
      LLVMValueRef cmp =
        LLVMBuildICmp(builder, LLVMIntNE, rem,
                      LLVMConstInt(LLVMTypeOf(address), 0, false), "");
      emitCondBrToBlock(cmp, bailoutBB);
    }

    // Check address valid.
    if (check->getFlags() & MemoryCheck::CheckAddress) {
      LLVMValueRef args[] = {
        threadParam,
        ramSizeLog2Param,
        address
      };
      LLVMValueRef isValid = emitCallToBeInlined(functions.jitCheckAddress,
                                                 args, 3);
      LLVMValueRef cmp =
        LLVMBuildICmp(builder, LLVMIntEQ, isValid,
                      LLVMConstInt(LLVMTypeOf(isValid), 0, false), "");
      emitCondBrToBlock(cmp, bailoutBB);
    }

    // Check invalidation info.
    if (check->getFlags() & MemoryCheck::CheckInvalidation) {
      LLVMValueRef args[] = {
        threadParam,
        address
      };
      LLVMValueRef cacheInvalidated =
        emitCallToBeInlined(getJitInvalidateFunction(check->getSize()), args,
                            2);
      LLVMValueRef cmp =
      LLVMBuildICmp(builder, LLVMIntNE, cacheInvalidated,
                    LLVMConstInt(LLVMTypeOf(cacheInvalidated), 0, false), "");
      emitCondBrToBlock(cmp, bailoutBB);
    }
    delete check;
  }
}

LLVMValueRef JITImpl::getJitInvalidateFunction(unsigned size)
{
  switch (size) {
  default:
    assert(0 && "Unexpected size");
  case 1:
    return functions.jitInvalidateByteCheck;
  case 2:
    return functions.jitInvalidateShortCheck;
  case 4:
    return functions.jitInvalidateWordCheck;
  }
}

InstFunction_t JITImpl::getFunctionThunk(JITFunctionInfo &info)
{
  LLVMValueRef f = LLVMAddFunction(module, "", jitFunctionType);
  LLVMValueRef thread = LLVMGetParam(f, 0);
  LLVMBasicBlockRef entryBB =
    LLVMAppendBasicBlockInContext(context, f, "entry");
  LLVMPositionBuilderAtEnd(builder, entryBB);
  LLVMValueRef args[] = {
    thread
  };
  LLVMValueRef call = LLVMBuildCall(builder, info.value, args, 1, "");
  LLVMSetTailCall(call, true);
  LLVMSetInstructionCallConv(call, LLVMFastCallConv);
  LLVMBuildRet(builder, call);
  if (DEBUG_JIT) {
    LLVMDumpValue(f);
    LLVMVerifyFunction(f, LLVMAbortProcessAction);
  }
  return reinterpret_cast<InstFunction_t>(
    LLVMGetPointerToGlobal(executionEngine, f));
}

bool JITImpl::invalidate(Core &core, uint32_t pc)
{
  JITCoreInfo *coreInfo = getJITCoreInfo(core);
  if (!coreInfo)
    return false;
  auto entry = coreInfo->functionMap.find(pc);
  if (entry == coreInfo->functionMap.end())
    return false;

  std::vector<JITFunctionInfo*> worklist;
  std::set<JITFunctionInfo*> toInvalidate;
  worklist.push_back(entry->second);
  toInvalidate.insert(entry->second);
  do {
    JITFunctionInfo *info = worklist.back();
    worklist.pop_back();
    for (JITFunctionInfo *ref : info->references) {
      if (!toInvalidate.insert(ref).second)
        continue;
      worklist.push_back(ref);
    }
  } while (!worklist.empty());
  for (JITFunctionInfo *funcInfo : toInvalidate) {
    uint32_t functionPc = funcInfo->pc;
    core.clearOpcode(functionPc);
    coreInfo->unreachableFunctions.push_back(functionPc);
  }
  return true;
}

#else

using namespace axe;

class axe::JITImpl {
public:
  static void initializeGlobalState() {}
  void init() {}
  void compileBlock(Core &core, uint32_t pc) {}
  bool invalidate(Core &core, uint32_t pc) { return false; }
};

#endif

JIT::JIT() {
  pImpl = new JITImpl;
}

JIT::~JIT() {
  delete pImpl;
}

void JIT::initializeGlobalState()
{
  JITImpl::initializeGlobalState();
}

void JIT::compileBlock(Core &core, uint32_t pc)
{
  return pImpl->compileBlock(core, pc);
}

bool JIT::invalidate(Core &core, uint32_t pc)
{
  return pImpl->invalidate(core, pc);
}
