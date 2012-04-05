// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

// TODO call LLVMDisposeBuilder(), other cleanup.

#define DEBUG_JIT false

#include "JIT.h"
#include "llvm-c/Core.h"
#include "llvm-c/BitReader.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Target.h"
#include "llvm-c/Transforms/Scalar.h"
#ifdef DEBUG_JIT
#include "llvm-c/Analysis.h"
#endif
#include "Instruction.h"
#include "Core.h"
#include "InstructionBitcode.h"
#include "LLVMExtra.h"
#include "InstructionProperties.h"
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <map>
#include <vector>

struct JITFunctionInfo {
  explicit JITFunctionInfo(uint32_t a) :
    shiftedAddress(a), value(0), func(0), isStub(false) {}
  JITFunctionInfo(uint32_t a, LLVMValueRef v, JITInstructionFunction_t f,
                  bool s) :
    shiftedAddress(a), value(v), func(f), isStub(s) {}
  uint32_t shiftedAddress;
  LLVMValueRef value;
  JITInstructionFunction_t func;
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
  for (std::map<uint32_t,JITFunctionInfo*>::iterator it = functionMap.begin(),
       e = functionMap.end(); it != e; ++it) {
    delete it->second;
  }
}

class JITImpl {
  bool initialized;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMExecutionEngineRef executionEngine;
  LLVMTypeRef jitFunctionType;
  LLVMValueRef jitStubImplFunction;
  LLVMValueRef jitGetPcFunction;
  LLVMValueRef jitUpdateExecutionFrequencyFunction;
  LLVMPassManagerRef FPM;

  std::map<const Core*,JITCoreInfo*> jitCoreMap;
  std::vector<LLVMValueRef> earlyReturnIncomingValues;
  std::vector<LLVMBasicBlockRef> earlyReturnIncomingBlocks;

  LLVMValueRef threadParam;
  LLVMBasicBlockRef earlyReturnBB;
  LLVMValueRef earlyReturnPhi;
  std::vector<LLVMValueRef> calls;

  void init();
  void resetPerFunctionState();
  void reclaimUnreachableFunctions(JITCoreInfo &coreInfo);
  void reclaimUnreachableFunctions();
  void checkReturnValue(LLVMValueRef call, LLVMValueRef f,
                        InstructionProperties &properties);
  void ensureEarlyReturnBB(LLVMTypeRef phiType);
  JITCoreInfo *getJITCoreInfo(const Core &);
  JITCoreInfo *getOrCreateJITCoreInfo(const Core &);
  JITFunctionInfo *getJITFunctionOrStubImpl(JITCoreInfo &coreInfo,
                                            uint32_t shiftedAddress);
  LLVMValueRef getJITFunctionOrStub(JITCoreInfo &coreIfno,
                                    uint32_t shiftedAddress,
                                    JITFunctionInfo *caller);
  bool compileOneFragment(Core &core, JITCoreInfo &coreInfo, uint32_t address,
                          bool &endOfBlock, uint32_t &nextAddress);
  void emitJumpToNextFragment(JITCoreInfo &coreInfo, uint32_t targetPc,
                              JITFunctionInfo *caller);
  bool emitJumpToNextFragment(InstructionOpcode opc, const Operands &operands,
                              JITCoreInfo &coreInfo, uint32_t shiftedNextAddress,
                              JITFunctionInfo *caller);
  JITInstructionFunction_t getFunctionThunk(JITFunctionInfo &info);
public:
  JITImpl() : initialized(false) {}
  ~JITImpl();
  static JITImpl instance;
  bool invalidate(Core &c, uint32_t shiftedAddress);
  void compileBlock(Core &core, uint32_t address);
};

JITImpl JITImpl::instance;

JITImpl::~JITImpl()
{
  for (std::map<const Core*,JITCoreInfo*>::iterator it = jitCoreMap.begin(),
       e = jitCoreMap.end(); it != e; ++it) {
    delete it->second;
  }
}

void JITImpl::init()
{
  if (initialized)
    return;
  LLVMLinkInJIT();
  LLVMInitializeNativeTarget();
  LLVMMemoryBufferRef memBuffer =
    LLVMExtraCreateMemoryBufferWithPtr(instructionBitcode,
                                       instructionBitcodeSize);
  char *outMessage;
  if (LLVMParseBitcode(memBuffer, &module, &outMessage)) {
    std::cerr << "Error loading bitcode: " << outMessage << '\n';
    std::abort();
  }
  // TODO experiment with opt level.
  if (LLVMCreateJITCompilerForModule(&executionEngine, module, 1,
                                      &outMessage)) {
    std::cerr << "Error creating JIT compiler: " << outMessage << '\n';
    std::abort();
  }
  builder = LLVMCreateBuilder();
  LLVMValueRef callee = LLVMGetNamedFunction(module, "jitInstructionTemplate");
  assert(callee && "jitInstructionTemplate() not found in module");
  jitStubImplFunction = LLVMGetNamedFunction(module, "jitStubImpl");
  assert(jitStubImplFunction && "jitStubImpl() not found in module");
  jitGetPcFunction = LLVMGetNamedFunction(module, "jitGetPc");
  assert(jitGetPcFunction && "jitGetPc() not found in module");
  jitUpdateExecutionFrequencyFunction =
    LLVMGetNamedFunction(module, "jitUpdateExecutionFrequency");
  assert(jitUpdateExecutionFrequencyFunction &&
         "jitUpdateExecutionFrequency() not found in module");
  jitFunctionType = LLVMGetElementType(LLVMTypeOf(callee));
  FPM = LLVMCreateFunctionPassManagerForModule(module);
  LLVMAddTargetData(LLVMGetExecutionEngineTargetData(executionEngine), FPM);
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

void JITImpl::resetPerFunctionState()
{
  threadParam = 0;
  earlyReturnBB = 0;
  earlyReturnIncomingValues.clear();
  earlyReturnIncomingBlocks.clear();
  calls.clear();
}

static bool
getInstruction(Core &core, uint32_t address, InstructionOpcode &opc,
               Operands &operands)
{
  if (!core.isValidAddress(address + core.ram_base))
    return false;
  instructionDecode(core, address >> 1, opc, operands);
  return true;
}

void JITImpl::reclaimUnreachableFunctions(JITCoreInfo &coreInfo)
{
  std::vector<uint32_t> &unreachableFunctions = coreInfo.unreachableFunctions;
  std::map<uint32_t,JITFunctionInfo*> &functionMap = coreInfo.functionMap;
  for (std::vector<uint32_t>::iterator it = unreachableFunctions.begin(),
       e = unreachableFunctions.end(); it != e; ++it) {
    std::map<uint32_t,JITFunctionInfo*>::iterator entry = functionMap.find(*it);
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
  for (std::map<const Core*,JITCoreInfo*>::iterator it = jitCoreMap.begin(),
       e = jitCoreMap.end(); it != e; ++it) {
    reclaimUnreachableFunctions(*it->second);
  }
}

static bool mayReturnEarly(InstructionProperties &properties)
{
  return properties.mayYield() || properties.mayEndTrace() ||
         properties.mayDeschedule();
}

void
JITImpl::checkReturnValue(LLVMValueRef call, LLVMValueRef f,
                          InstructionProperties &properties)
{
  if (!mayReturnEarly(properties))
    return;
  LLVMValueRef cmp =
    LLVMBuildICmp(builder, LLVMIntNE, call,
                  LLVMConstInt(LLVMTypeOf(call), 0, JIT_RETURN_CONTINUE), "");
  LLVMBasicBlockRef afterBB = LLVMAppendBasicBlock(f, "");
  ensureEarlyReturnBB(LLVMTypeOf(call));
  LLVMBuildCondBr(builder, cmp, earlyReturnBB, afterBB);
  earlyReturnIncomingBlocks.push_back(LLVMGetInsertBlock(builder));
  earlyReturnIncomingValues.push_back(call);
  LLVMPositionBuilderAtEnd(builder, afterBB);
}

void JITImpl::ensureEarlyReturnBB(LLVMTypeRef phiType)
{
  if (earlyReturnBB)
    return;
  // Save off current insert point.
  LLVMBasicBlockRef savedBB = LLVMGetInsertBlock(builder);
  LLVMValueRef f = LLVMGetBasicBlockParent(savedBB);
  earlyReturnBB = LLVMAppendBasicBlock(f, "early_return");
  LLVMPositionBuilderAtEnd(builder, earlyReturnBB);
  // Create phi (incoming values will be filled in later).
  earlyReturnPhi = LLVMBuildPhi(builder, phiType, "");
  LLVMBuildRet(builder, earlyReturnPhi);
  // Restore insert point.
  LLVMPositionBuilderAtEnd(builder, savedBB);
}

void JITImpl::compileBlock(Core &core, uint32_t address)
{
  init();
  reclaimUnreachableFunctions();
  bool endOfBlock;
  uint32_t nextAddress;
  JITCoreInfo &coreInfo = *getOrCreateJITCoreInfo(core);
  do {
    compileOneFragment(core, coreInfo, address, endOfBlock, nextAddress);
    address = nextAddress;
  } while (!endOfBlock);
}

static bool
getSuccessors(InstructionOpcode opc, const Operands &operands,
              uint32_t shiftedNextAddress, std::set<uint32_t> &successors)
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
    successors.insert(shiftedNextAddress);
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
    successors.insert(shiftedNextAddress);
    return true;
  }
}

JITCoreInfo *JITImpl::getJITCoreInfo(const Core &c)
{
  std::map<const Core*,JITCoreInfo*>::iterator it = jitCoreMap.find(&c);
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
getJITFunctionOrStubImpl(JITCoreInfo &coreInfo, uint32_t shiftedAddress)
{
  JITFunctionInfo *&info = coreInfo.functionMap[shiftedAddress];
  if (info)
    return info;
  LLVMBasicBlockRef savedInsertPoint = LLVMGetInsertBlock(builder);
  LLVMValueRef f = LLVMAddFunction(module, "", jitFunctionType);
  LLVMSetFunctionCallConv(f, LLVMFastCallConv);
  LLVMBasicBlockRef entryBB = LLVMAppendBasicBlock(f, "entry");
  LLVMPositionBuilderAtEnd(builder, entryBB);
  LLVMValueRef args[] = {
    LLVMGetParam(f, 0)
  };
  LLVMValueRef call =
    LLVMBuildCall(builder, jitStubImplFunction, args, 1, "");
  LLVMBuildRet(builder, call);
  if (DEBUG_JIT) {
    LLVMDumpValue(f);
    LLVMVerifyFunction(f, LLVMAbortProcessAction);
  }
  JITInstructionFunction_t code =
    reinterpret_cast<JITInstructionFunction_t>(
     LLVMGetPointerToGlobal(executionEngine, f));
  info = new JITFunctionInfo(shiftedAddress, f, code, true);
  LLVMPositionBuilderAtEnd(builder, savedInsertPoint);
  return info;
}

LLVMValueRef JITImpl::
getJITFunctionOrStub(JITCoreInfo &coreInfo, uint32_t shiftedAddress,
                     JITFunctionInfo *caller)
{
  JITFunctionInfo *info = getJITFunctionOrStubImpl(coreInfo, shiftedAddress);
  info->references.insert(caller);
  return info->value;
}

LLVMBasicBlockRef
appendBBToCurrentFunction(LLVMBuilderRef builder, const char *name)
{
  LLVMBasicBlockRef currentBB = LLVMGetInsertBlock(builder);
  LLVMValueRef function = LLVMGetBasicBlockParent(currentBB);
  return LLVMAppendBasicBlock(function, name);
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
                       JITCoreInfo &coreInfo, uint32_t shiftedNextAddress,
                       JITFunctionInfo *caller)
{
  std::set<uint32_t> successors;
  if (!getSuccessors(opc, operands, shiftedNextAddress, successors))
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
    LLVMValueRef nextPc = LLVMBuildCall(builder, jitGetPcFunction, args, 1, "");
    calls.push_back(nextPc);
    for (;it != successors.end(); ++it) {
      LLVMValueRef cmp =
        LLVMBuildICmp(builder, LLVMIntEQ, nextPc,
                      LLVMConstInt(LLVMTypeOf(nextPc), *it, false), "");
      LLVMBasicBlockRef trueBB = appendBBToCurrentFunction(builder, "");
      LLVMBasicBlockRef afterBB = appendBBToCurrentFunction(builder, "");
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
    instructionTransform(opc, ops, core, address >> 1);
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
compileOneFragment(Core &core, JITCoreInfo &coreInfo, uint32_t startAddress,
                   bool &endOfBlock, uint32_t &addressAfterFragment)
{
  assert(initialized);
  resetPerFunctionState();

  std::map<uint32_t,JITFunctionInfo*>::iterator infoIt =
    coreInfo.functionMap.find(startAddress >> 1);
  JITFunctionInfo *info =
    (infoIt == coreInfo.functionMap.end()) ? 0 : infoIt->second;
  if (info && !info->isStub) {
    endOfBlock = true;
    return false;
  }

  std::vector<InstructionOpcode> opcode;
  std::vector<Operands> operands;
  if (!getFragmentToCompile(core, startAddress, opcode, operands,
                            endOfBlock, addressAfterFragment))
    return false;

  LLVMValueRef f;
  if (info) {
    f = info->value;
    info->func = 0;
    info->isStub = false;
    deleteFunctionBody(f);
  } else {
    info = new JITFunctionInfo(startAddress >> 1);
    coreInfo.functionMap.insert(std::make_pair(startAddress >> 1,
                                               info));
    // Create function to contain the code we are about to add.
    info->value = f = LLVMAddFunction(module, "", jitFunctionType);
    LLVMSetFunctionCallConv(f, LLVMFastCallConv);
  }
  threadParam = LLVMGetParam(f, 0);
  LLVMValueRef ramBase = LLVMConstInt(LLVMInt32Type(), core.ram_base, false);
  LLVMValueRef ramSizeLog2 = LLVMConstInt(LLVMInt32Type(), core.ramSizeLog2,
                                          false);
  LLVMBasicBlockRef entryBB = LLVMAppendBasicBlock(f, "entry");
  LLVMPositionBuilderAtEnd(builder, entryBB);
  uint32_t address = startAddress;
  bool needsReturn = true;
  for (unsigned i = 0, e = opcode.size(); i != e; ++i) {
    InstructionOpcode opc = opcode[i];
    const Operands &ops = operands[i];
    InstructionProperties *properties = &instructionProperties[opc];
    uint32_t nextAddress = address + properties->size;

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
    args[1] = LLVMConstInt(paramTypes[1], nextAddress >> 1, false);
    args[2] = ramBase;
    args[3] = ramSizeLog2;
    for (unsigned i = fixedArgs; i < numArgs; i++) {
      uint32_t value =
      properties->getNumExplicitOperands() <= 3 ? ops.ops[i - fixedArgs] :
      ops.lops[i - fixedArgs];
      args[i] = LLVMConstInt(paramTypes[i], value, false);
    }
    LLVMValueRef call = LLVMBuildCall(builder, callee, args, numArgs, "");
    calls.push_back(call);
    checkReturnValue(call, f, *properties);
    if (properties->mayBranch() && properties->function &&
        emitJumpToNextFragment(opc, ops, coreInfo, nextAddress >> 1,
                               info)) {
      needsReturn = false;
    }
    address = nextAddress;
  }
  if (needsReturn) {
    LLVMValueRef args[] = {
      threadParam
    };
    LLVMValueRef call =
      LLVMBuildCall(builder, jitUpdateExecutionFrequencyFunction, args, 1, "");
    calls.push_back(call);
    // Build return.
    LLVMBuildRet(builder,
                 LLVMConstInt(LLVMGetReturnType(jitFunctionType),
                              JIT_RETURN_CONTINUE, 0));
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
  for (std::vector<LLVMValueRef>::iterator it = calls.begin(), e = calls.end();
       it != e; ++it) {
    LLVMExtraInlineFunction(*it);
  }
  LLVMRunFunctionPassManager(FPM, f);
  if (DEBUG_JIT) {
    LLVMDumpValue(f);
  }
  // Compile.
  JITInstructionFunction_t compiledFunction =
    reinterpret_cast<JITInstructionFunction_t>(
      LLVMRecompileAndRelinkFunction(executionEngine, f));
  info->isStub = false;
  info->func = compiledFunction;
  core.setOpcode(startAddress >> 1, getFunctionThunk(*info),
                 address - startAddress);
  return true;
}

JITInstructionFunction_t JITImpl::getFunctionThunk(JITFunctionInfo &info)
{
  LLVMValueRef f = LLVMAddFunction(module, "", jitFunctionType);
  LLVMValueRef thread = LLVMGetParam(f, 0);
  LLVMBasicBlockRef entryBB = LLVMAppendBasicBlock(f, "entry");
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
  return reinterpret_cast<JITInstructionFunction_t>(
    LLVMGetPointerToGlobal(executionEngine, f));
}

bool JITImpl::invalidate(Core &core, uint32_t shiftedAddress)
{
  JITCoreInfo *coreInfo = getJITCoreInfo(core);
  if (!coreInfo)
    return false;
  std::map<uint32_t,JITFunctionInfo*>::iterator entry =
    coreInfo->functionMap.find(shiftedAddress);
  if (entry == coreInfo->functionMap.end())
    return false;

  std::vector<JITFunctionInfo*> worklist;
  std::set<JITFunctionInfo*> toInvalidate;
  worklist.push_back(entry->second);
  toInvalidate.insert(entry->second);
  do {
    JITFunctionInfo *info = worklist.back();
    worklist.pop_back();
    for (std::set<JITFunctionInfo*>::iterator it = info->references.begin(),
         e = info->references.end(); it != e; ++it) {
      if (!toInvalidate.insert(*it).second)
        continue;
      worklist.push_back(*it);
    }
  } while (!worklist.empty());
  for (std::set<JITFunctionInfo*>::iterator it = toInvalidate.begin(),
       e = toInvalidate.end(); it != e; ++it) {
    uint32_t shiftedAddress = (*it)->shiftedAddress;
    core.clearOpcode(shiftedAddress);
    coreInfo->unreachableFunctions.push_back(shiftedAddress);
  }
  return true;
}

void JIT::compileBlock(Core &core, uint32_t address)
{
  return JITImpl::instance.compileBlock(core, address);
}

bool JIT::invalidate(Core &core, uint32_t shiftedAddress)
{
  return JITImpl::instance.invalidate(core, shiftedAddress);
}
