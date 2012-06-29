// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "DecodeCache.h"
#include "Thread.h"
#include "Trace.h"

DecodeCache::DecodeCache(uint32_t sz, uint32_t b)
{
  state.size = sz;
  state.base = b;
  state.operands = new Operands[sz];
  state.opcode = new OPCODE_TYPE[sz + ILLEGAL_PC_THREAD_ADDR_OFFSET];
  state.executionFrequency = new executionFrequency_t[sz];
  std::memset(state.executionFrequency, 0,
              sizeof(state.executionFrequency[0]) * sz);
  initCache(Tracer::get().getTracingEnabled());
}

DecodeCache::~DecodeCache()
{
  delete[] state.operands;
  delete[] state.opcode;
  delete[] state.executionFrequency;
}

void DecodeCache::State::clearOpcode(uint32_t pc)
{
  opcode[pc] = getInstruction_DECODE(Tracer::get().getTracingEnabled());
}

void DecodeCache::initCache(bool tracing)
{
  // Initialise instruction cache.
  OPCODE_TYPE decode = getInstruction_DECODE(tracing);
  for (unsigned i = 0; i < state.size; i++) {
    state.opcode[i] = decode;
  }
  state.opcode[state.size] = getInstruction_ILLEGAL_PC(tracing);
  state.opcode[state.getRunJitAddr()] = getInstruction_RUN_JIT(tracing);
  state.opcode[state.getInterpretOneAddr()] = getInstruction_INTERPRET_ONE(tracing);
  state.opcode[state.getIllegalPCThreadAddr()] =
    getInstruction_ILLEGAL_PC_THREAD(tracing);
}
