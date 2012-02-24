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

class JITImpl {
  bool initialized;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMExecutionEngineRef executionEngine;
  LLVMTypeRef jitFunctionType;
  LLVMValueRef jitEarlyReturnFunction;
  LLVMPassManagerRef FPM;
  std::vector<JITInstructionFunction_t> unreachableFunctions;
  std::vector<LLVMValueRef> earlyReturnIncomingValues;
  std::vector<LLVMBasicBlockRef> earlyReturnIncomingBlocks;
  std::map<JITInstructionFunction_t,LLVMValueRef> functionPtrMap;

  LLVMValueRef threadParam;
  LLVMBasicBlockRef earlyReturnBB;
  LLVMValueRef earlyReturnPhi;

  void init();
  void resetPerFunctionState();
  void reclaimUnreachableFunctions();
  void checkReturnValue(LLVMValueRef call, LLVMValueRef f,
                        InstructionProperties &properties);
  void ensureEarlyReturnBB(LLVMTypeRef phiType);

  bool compileOneFragment(Core &core, uint32_t address,
                          JITInstructionFunction_t &out, bool &endOfBlock,
                          uint32_t &nextAddress);
public:
  JITImpl() : initialized(false) {}
  static JITImpl instance;
  void markUnreachable(const JITInstructionFunction_t f);
  void compileBlock(Core &core, uint32_t address);
};

JITImpl JITImpl::instance;

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
  jitEarlyReturnFunction = LLVMGetNamedFunction(module,
                                                "jitEarlyReturnFunction");
  assert(jitEarlyReturnFunction &&
         "jitEarlyReturnFunction() not found in module");

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
  for (std::vector<JITInstructionFunction_t>::iterator it = unreachableFunctions.begin(),
       e = unreachableFunctions.end(); it != e; ++it) {
    std::map<JITInstructionFunction_t,LLVMValueRef>::iterator entry =
      functionPtrMap.find(*it);
    assert(entry != functionPtrMap.end());
    LLVMFreeMachineCodeForFunction(executionEngine, entry->second);
    LLVMDeleteFunction(entry->second);
    functionPtrMap.erase(entry);
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
  // Call jitEarlyReturnFunction.
  LLVMValueRef args[] = {
    threadParam,
    earlyReturnPhi
  };
  LLVMBuildCall(builder, jitEarlyReturnFunction, args, 2, "");
  // Return
  LLVMBuildRetVoid(builder);
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
  JITInstructionFunction_t out;
  do {
    if (compileOneFragment(core, address, out, endOfBlock, nextAddress)) {
      core.opcode[address >> 1] = core.getJitFunctionOpcode();
      core.operands[address >> 1].func = out;
    }
    address = nextAddress;
  } while (!endOfBlock);
}

/// Try and compile a fragment starting at the specified address. Returns
/// true if successful setting \a nextAddress to the first instruction after
/// the fragment. If unsuccessful returns false and sets \a nextAddress to the
/// address after the current function. \a endOfBlock is set to true if the
/// next address is in a new basic block.
bool JITImpl::
compileOneFragment(Core &core, uint32_t address, JITInstructionFunction_t &out,
                   bool &endOfBlock, uint32_t &nextAddress)
{
  assert(initialized);
  resetPerFunctionState();
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
  // Create function to contain the code we are about to add.
  LLVMValueRef f = LLVMAddFunction(module, "", jitFunctionType);
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
    unsigned nextAddress = address + properties->size;
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
  // Build return.
  LLVMBuildRetVoid(builder);
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
  out = reinterpret_cast<JITInstructionFunction_t>(
                                                   LLVMGetPointerToGlobal(executionEngine, f));
  functionPtrMap.insert(std::make_pair(out, f));
  return true;
}

void JITImpl::markUnreachable(const JITInstructionFunction_t f)
{
  assert(initialized);
  unreachableFunctions.push_back(f);
}

void JIT::compileBlock(Core &core, uint32_t address)
{
  return JITImpl::instance.compileBlock(core, address);
}

void JIT::markUnreachable(const JITInstructionFunction_t f)
{
  JITImpl::instance.markUnreachable(f);
}
