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

class JITImpl {
  bool initialized;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMExecutionEngineRef executionEngine;
  LLVMTypeRef jitFunctionType;
  LLVMPassManagerRef FPM;
  std::vector<uint32_t> unreachableFunctions;
  std::vector<LLVMValueRef> earlyReturnIncomingValues;
  std::vector<LLVMBasicBlockRef> earlyReturnIncomingBlocks;
  std::map<uint32_t,JITFunctionInfo*> jitFunctionMap;

  LLVMValueRef threadParam;
  LLVMBasicBlockRef earlyReturnBB;
  LLVMValueRef earlyReturnPhi;

  void init();
  void resetPerFunctionState();
  void reclaimUnreachableFunctions();
  void checkReturnValue(LLVMValueRef call, LLVMValueRef f,
                        InstructionProperties &properties);
  void ensureEarlyReturnBB(LLVMTypeRef phiType);
  JITFunctionInfo *getJITFunctionOrStubImpl(uint32_t shiftedAddress);
  LLVMValueRef getJITFunctionOrStub(uint32_t shiftedAddress,
                                    JITFunctionInfo *caller);
  bool compileOneFragment(Core &core, uint32_t address, bool &endOfBlock,
                          uint32_t &nextAddress);
  bool emitJumpToNextFragment(InstructionOpcode &opc, Operands &operands,
                              uint32_t shiftedNextAddress,
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
  for (std::map<uint32_t,JITFunctionInfo*>::iterator it = jitFunctionMap.begin(),
       e = jitFunctionMap.end(); it != e; ++it) {
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
  if (LLVMCreateJITCompilerForModule(&executionEngine, module, 2,
                                      &outMessage)) {
    std::cerr << "Error creating JIT compiler: " << outMessage << '\n';
    std::abort();
  }
  builder = LLVMCreateBuilder();
  LLVMValueRef callee = LLVMGetNamedFunction(module, "jitInstructionTemplate");
  assert(callee && "jitInstructionTemplate() not found in module");
  jitFunctionType = LLVMGetElementType(LLVMTypeOf(callee));
  FPM = LLVMCreateFunctionPassManagerForModule(module);
  LLVMAddTargetData(LLVMGetExecutionEngineTargetData(executionEngine), FPM);
  LLVMAddBasicAliasAnalysisPass(FPM);
  LLVMAddGVNPass(FPM);
  LLVMAddDeadStoreEliminationPass(FPM);
  LLVMAddInstructionCombiningPass(FPM);
  LLVMAddCFGSimplificationPass(FPM);
  LLVMExtraAddDeadCodeEliminationPass(FPM);
  LLVMInitializeFunctionPassManager(FPM);
  initialized = true;
}

void JITImpl::resetPerFunctionState()
{
  threadParam = 0;
  earlyReturnBB = 0;
  earlyReturnIncomingValues.clear();
  earlyReturnIncomingBlocks.clear();
}

static bool
getInstruction(Core &core, uint32_t address, InstructionOpcode &opc, 
               Operands &operands)
{
  if (!core.isValidAddress(address))
    return false;
  instructionDecode(core, address >> 1, opc, operands);
  return true;
}

void JITImpl::reclaimUnreachableFunctions()
{
  for (std::vector<uint32_t>::iterator it = unreachableFunctions.begin(),
       e = unreachableFunctions.end(); it != e; ++it) {
    std::map<uint32_t,JITFunctionInfo*>::iterator entry =
      jitFunctionMap.find(*it);
    if (entry == jitFunctionMap.end())
      continue;
    LLVMValueRef value = entry->second->value;
    LLVMFreeMachineCodeForFunction(executionEngine, value);
    LLVMReplaceAllUsesWith(value, LLVMGetUndef(LLVMTypeOf(value)));
    LLVMDeleteFunction(value);
    delete entry->second;
    jitFunctionMap.erase(entry);
  }
  unreachableFunctions.clear();
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
  if (!unreachableFunctions.empty())
    reclaimUnreachableFunctions();
  bool endOfBlock;
  uint32_t nextAddress;
  do {
    compileOneFragment(core, address, endOfBlock, nextAddress);
    address = nextAddress;
  } while (!endOfBlock);
}

static bool
getSuccessors(InstructionOpcode &opc, Operands &operands,
              uint32_t shiftedNextAddress, std::set<uint32_t> &successors)
{
  switch (opc) {
  default: return false;
  case BRFT_ru6:
  case BRFT_lru6:
  case BRBT_ru6:
  case BRBT_lru6:
  case BRFF_ru6:
  case BRFF_lru6:
  case BRBF_ru6:
  case BRBF_lru6:
      assert(shiftedNextAddress != operands.ops[1]);
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
  }
}

JITFunctionInfo *JITImpl::getJITFunctionOrStubImpl(uint32_t shiftedAddress)
{
  JITFunctionInfo *&info = jitFunctionMap[shiftedAddress];
  if (info)
    return info;
  LLVMBasicBlockRef savedInsertPoint = LLVMGetInsertBlock(builder);
  LLVMValueRef f = LLVMAddFunction(module, "", jitFunctionType);
  LLVMSetFunctionCallConv(f, LLVMFastCallConv);
  LLVMBasicBlockRef entryBB = LLVMAppendBasicBlock(f, "entry");
  LLVMPositionBuilderAtEnd(builder, entryBB);
  LLVMBuildRet(builder,
               LLVMConstInt(LLVMGetReturnType(jitFunctionType),
                            JIT_RETURN_CONTINUE, 0));
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
getJITFunctionOrStub(uint32_t shiftedAddress, JITFunctionInfo *caller)
{
  JITFunctionInfo *info = getJITFunctionOrStubImpl(shiftedAddress);
  info->references.insert(caller);
  return info->value;
}

bool JITImpl::
emitJumpToNextFragment(InstructionOpcode &opc, Operands &operands,
                       uint32_t shiftedNextAddress, JITFunctionInfo *caller)
{
  std::set<uint32_t> successors;
  if (!getSuccessors(opc, operands, shiftedNextAddress, successors))
    return false;
  // TODO conditional branches.
  if (successors.size() != 1)
    return false;
  LLVMValueRef next = getJITFunctionOrStub(*successors.begin(), caller);
  LLVMValueRef args[] = {
    threadParam
  };
  LLVMValueRef call = LLVMBuildCall(builder, next, args, 1, "");
  LLVMSetTailCall(call, true);
  LLVMSetInstructionCallConv(call, LLVMFastCallConv);
  LLVMBuildRet(builder, call);
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

/// Try and compile a fragment starting at the specified address. Returns
/// true if successful setting \a nextAddress to the first instruction after
/// the fragment. If unsuccessful returns false and sets \a nextAddress to the
/// address after the current function. \a endOfBlock is set to true if the
/// next address is in a new basic block.
bool JITImpl::
compileOneFragment(Core &core, uint32_t startAddress,
                   bool &endOfBlock, uint32_t &nextAddress)
{
  assert(initialized);
  resetPerFunctionState();

  uint32_t address = startAddress;

  std::map<uint32_t,JITFunctionInfo*>::iterator infoIt =
    jitFunctionMap.find(startAddress >> 1);
  JITFunctionInfo *info =
    (infoIt == jitFunctionMap.end()) ? 0 : infoIt->second;
  if (info && !info->isStub) {
    endOfBlock = true;
    return false;
  }

  endOfBlock = false;
  nextAddress = address;

  InstructionOpcode opc;
  Operands operands;
  if (!getInstruction(core, address, opc, operands)) {
    endOfBlock = true;
    return false;
  }
  instructionTransform(opc, operands, core, address >> 1);
  InstructionProperties *properties = &instructionProperties[opc];
  nextAddress = address + properties->size;
  // There isn't much point JITing a single instruction.
  if (properties->mayBranch()) {
    endOfBlock = true;
    return false;
  }
  // Check if we can JIT the instruction.
  if (!properties->function) {
    return false;
  }
  LLVMValueRef f;
  if (info) {
    f = info->value;
    info->func = 0;
    info->isStub = false;
    deleteFunctionBody(f);
  } else {
    info = new JITFunctionInfo(address >> 1);
    jitFunctionMap.insert(std::make_pair(address >> 1, info));
    // Create function to contain the code we are about to add.
    info->value = f = LLVMAddFunction(module, "", jitFunctionType);
    LLVMSetFunctionCallConv(f, LLVMFastCallConv);
  }
  threadParam = LLVMGetParam(f, 0);
  LLVMBasicBlockRef entryBB = LLVMAppendBasicBlock(f, "entry");
  LLVMPositionBuilderAtEnd(builder, entryBB);
  std::vector<LLVMValueRef> calls;
  do {
    // Lookup function to call.
    LLVMValueRef callee = LLVMGetNamedFunction(module, properties->function);
    assert(callee && "Function for instruction not found in module");
    LLVMTypeRef calleeType = LLVMGetElementType(LLVMTypeOf(callee));
    unsigned numArgs = properties->numExplicitOperands + 2;
    nextAddress = address + properties->size;
    assert(LLVMCountParamTypes(calleeType) == numArgs);
    LLVMTypeRef paramTypes[8];
    assert(numArgs <= 8);
    LLVMGetParamTypes(calleeType, paramTypes);
    // Build call.
    LLVMValueRef args[8];
    args[0] = threadParam;
    args[1] = LLVMConstInt(paramTypes[1], nextAddress >> 1, false);
    for (unsigned i = 2; i < numArgs; i++) {
      uint32_t value =
      properties->numExplicitOperands <= 3 ? operands.ops[i - 2] :
      operands.lops[i - 2];
      args[i] = LLVMConstInt(paramTypes[i], value, false);
    }
    LLVMValueRef call = LLVMBuildCall(builder, callee, args, numArgs, "");
    calls.push_back(call);
    checkReturnValue(call, f, *properties);
    // Update invalidation info.
    if (calls.size() == 1) {
      if (core.invalidationInfo[address >> 1] == Core::INVALIDATE_NONE) {
        core.invalidationInfo[address >> 1] = Core::INVALIDATE_CURRENT;
      }
    } else {
      core.invalidationInfo[address >> 1] =
      Core::INVALIDATE_CURRENT_AND_PREVIOUS;
    }
    if (properties->size != 2) {
      assert(properties->size == 4 && "Unexpected instruction size");
      core.invalidationInfo[(address >> 1) + 1] =
      Core::INVALIDATE_CURRENT_AND_PREVIOUS;
    }
    // See if we can JIT the next instruction.
    if (properties->mayBranch())
      break;
    // Increment address.
    address = nextAddress;
    if (!getInstruction(core, address, opc, operands)) {
      endOfBlock = true;
      break;
    }
    instructionTransform(opc, operands, core, address >> 1);
    properties = &instructionProperties[opc];
    nextAddress = address + properties->size;
    if (properties->mayBranch())
      endOfBlock = true;
  } while (properties->function);
  if (properties->mayBranch() && properties->function &&
      emitJumpToNextFragment(opc, operands, nextAddress >> 1, info)) {
    // Nothing
  } else {
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
  // Compile.
  JITInstructionFunction_t compiledFunction =
    reinterpret_cast<JITInstructionFunction_t>(
      LLVMRecompileAndRelinkFunction(executionEngine, f));
  info->isStub = false;
  info->func = compiledFunction;
  core.opcode[startAddress >> 1] = getFunctionThunk(*info);
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
  std::map<uint32_t,JITFunctionInfo*>::iterator entry =
    jitFunctionMap.find(shiftedAddress);
  if (entry == jitFunctionMap.end())
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
    core.opcode[shiftedAddress] = core.getDecodeOpcode();
    unreachableFunctions.push_back(shiftedAddress);
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
