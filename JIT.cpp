// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

// TODO call LLVMDisposeBuilder(), other cleanup.

#include "JIT.h"
#include "llvm-c/Core.h"
#include "llvm-c/BitReader.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Target.h"
#include "llvm-c/Transforms/Scalar.h"
#include "Instruction.h"
#include "Core.h"
#include "InstructionBitcode.h"
#include "LLVMExtra.h"
#include "InstructionProperties.h"
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <map>

static LLVMModuleRef module;
static LLVMBuilderRef builder;
static LLVMExecutionEngineRef executionEngine;
static LLVMTypeRef jitFunctionType;
static LLVMPassManagerRef FPM;
static std::vector<JITInstructionFunction_t> unreachableFunctions;
static std::map<JITInstructionFunction_t,LLVMValueRef> functionPtrMap;

void JIT::init()
{
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
  LLVMExtraAddDeadCodeEliminationPass(FPM);
  LLVMInitializeFunctionPassManager(FPM);
}

static bool
readInstMem(Core &core, uint32_t address, uint16_t &low, uint16_t &high,
        bool &highValid)
{
  if (!core.isValidAddress(address))
    return false;
  low = core.loadShort(address);
  if (core.isValidAddress(address + 2)) {
    high = core.loadShort(address + 2);
    highValid = true;
  } else {
    highValid = false;
  }
  return true;
}

static void reclaimUnreachableFunctions()
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

static void
checkReturnValue(LLVMValueRef call, LLVMValueRef f,
                 InstructionProperties &properties)
{
  if (!properties.mayYield() && !properties.mayEndTrace())
    return;
  LLVMValueRef cmp =
    LLVMBuildICmp(builder, LLVMIntNE, call,
                  LLVMConstInt(LLVMTypeOf(call), 0, JIT_RETURN_CONTINUE), "");
  LLVMBasicBlockRef returnBB = LLVMAppendBasicBlock(f, "");
  LLVMBasicBlockRef afterBB = LLVMAppendBasicBlock(f, "");
  LLVMBuildCondBr(builder, cmp, returnBB, afterBB);
  LLVMPositionBuilderAtEnd(builder, returnBB);
  LLVMValueRef retval;
  if (properties.mayYield() && properties.mayEndTrace()) {
    LLVMValueRef isYield =
      LLVMBuildICmp(builder, LLVMIntEQ, call,
                    LLVMConstInt(LLVMTypeOf(call), 0, JIT_RETURN_YIELD), "");
    LLVMValueRef yieldRetval =
      LLVMConstInt(LLVMGetReturnType(jitFunctionType), 1, false);
    LLVMValueRef endTraceRetval =
      LLVMConstInt(LLVMGetReturnType(jitFunctionType), 0, false);
    retval = LLVMBuildSelect(builder, isYield, yieldRetval, endTraceRetval, "");
  } else if (properties.mayYield()) {
    retval = LLVMConstInt(LLVMGetReturnType(jitFunctionType), 1, false);
  } else {
    retval = LLVMConstInt(LLVMGetReturnType(jitFunctionType), 0, false);
    assert(properties.mayEndTrace());
  }
  LLVMBuildRet(builder, retval);
  LLVMPositionBuilderAtEnd(builder, afterBB);
}

bool JIT::compile(Core &core, uint32_t address, JITInstructionFunction_t &out)
{
  InstructionOpcode opc;
  uint16_t low, high;
  bool highValid;
  if (!unreachableFunctions.empty())
    reclaimUnreachableFunctions();
  if (!readInstMem(core, address, low, high, highValid))
    return false;
  Operands operands;
  instructionDecode(low, high, highValid, opc, operands);
  instructionTransform(opc, operands, core, address >> 1);
  InstructionProperties *properties = &instructionProperties[opc];
  // Check if we can JIT the instruction.
  if (!properties->function)
    return false;
  // There isn't much point JITing a single instruction.
  if (properties->mayBranch())
    return false;
  // Create function to contain the code we are about to add.
  LLVMValueRef f = LLVMAddFunction(module, "", jitFunctionType);
  LLVMValueRef threadParam = LLVMGetParam(f, 0);
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
    address += properties->size;
    if (!readInstMem(core, address, low, high, highValid))
      break;
    instructionDecode(low, high, highValid, opc, operands);
    instructionTransform(opc, operands, core, address >> 1);
    properties = &instructionProperties[opc];
  } while (properties->function);
  // Build return.
  LLVMBuildRet(builder,
               LLVMConstInt(LLVMGetReturnType(jitFunctionType), 0, false));
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

void JIT::markUnreachable(const JITInstructionFunction_t f)
{
  unreachableFunctions.push_back(f);
}
