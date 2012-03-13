// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include <cstdlib>
#include "Thread.h"
#include "Core.h"
#include "Chanend.h"
#include "BitManip.h"
#include "CRC.h"
#include "InstructionHelpers.h"
#include "Exceptions.h"
#include "Synchroniser.h"
#include "InstructionMacrosCommon.h"
#include "SyscallHandler.h"
#include <cstdio>

/// Use to get the required LLVM type for instruction functions.
extern "C" JITReturn jitInstructionTemplate(Thread &t) {
  return JIT_RETURN_CONTINUE;
}

extern "C" uint32_t jitGetPc(Thread &t) {
  return t.pc;
}

extern "C" JITReturn jitStubImpl(Thread &t) {
  if (t.getParent().updateExecutionFrequencyFromStub(t.pc)) {
    t.pendingPc = t.pc;
    t.pc = t.getParent().getRunJitAddr();
  }
  return JIT_RETURN_END_TRACE;
}

extern "C" void jitUpdateExecutionFrequency(Thread &t) {
  t.getParent().updateExecutionFrequency(t.pc);
}

#define THREAD thread
#define CORE THREAD.getParent()
//#define ERROR() internalError(THREAD, __FILE__, __LINE__);
#define ERROR() std::abort();
#define OP(n) (field ## n)
#define LOP(n) OP(n)
#define EMIT_JIT_INSTRUCTION_FUNCTIONS
#include "InstructionGenOutput.inc"
#undef EMIT_JIT_INSTRUCTION_FUNCTIONS
