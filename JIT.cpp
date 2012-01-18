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

static LLVMModuleRef module;
static LLVMBuilderRef builder;
static LLVMExecutionEngineRef executionEngine;
static LLVMTypeRef jitFunctionType;
static LLVMPassManagerRef FPM;

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
  InstructionProperties &properties = instructionProperties[GETID_0r];
  LLVMValueRef callee = LLVMGetNamedFunction(module, properties.function);
  assert(callee && "Function for GETID_0r not found in module");
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

bool JIT::compile(Core &core, uint32_t address, void (*&out)(Thread &))
{
  InstructionOpcode opc;
  uint16_t low, high;
  bool highValid;
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
  LLVMBuildRetVoid(builder);
  // Optimize.
  for (std::vector<LLVMValueRef>::iterator it = calls.begin(), e = calls.end();
       it != e; ++it) {
    LLVMExtraInlineFunction(*it);
  }
  LLVMRunFunctionPassManager(FPM, f);
  // Compile.
  out = reinterpret_cast<void (*)(Thread &)>(
          LLVMGetPointerToGlobal(executionEngine, f));
  return true;
}
