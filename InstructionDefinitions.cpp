// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include <cstdlib>
#include "Thread.h"
#include "Core.h"
#include "BitManip.h"
#include "CRC.h"
#include "InstructionHelpers.h"
#include "Exceptions.h"
#include "Synchroniser.h"
#include "InstructionMacrosCommon.h"
#include <cstdio>

/// Use to get the required LLVM type for instruction functions.
extern "C" void jitInstructionTemplate(Thread &t, uint32_t pc) {
}

extern "C" void jitEarlyReturnFunction(Thread &t, JITReturn returnCode) {
  switch (returnCode) {
  default:
    break;
  case JIT_RETURN_END_TRACE:
    break;
  case JIT_RETURN_YIELD:
    t.pendingPc = t.pc;
    t.pc = t.getParent().getYieldAddr();
    break;
  case JIT_RETURN_DESCHEDULE:
    t.pendingPc = t.pc;
    t.pc = t.getParent().getDescheduleAddr();
    break;
  }
}

#define THREAD thread
#define CORE THREAD.getParent()
//#define ERROR() internalError(THREAD, __FILE__, __LINE__);
#define ERROR() std::abort();
#define OP(n) (field ## n)
#define LOP(n) OP(n)
#define EMIT_INSTRUCTION_FUNCTIONS
#include "InstructionGenOutput.inc"
#undef EMIT_INSTRUCTION_FUNCTIONS
