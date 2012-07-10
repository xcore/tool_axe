// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "DecodeCache.h"
#include "Thread.h"
#include "Trace.h"
#include <cstring>

DecodeCache::DecodeCache(uint32_t sz, uint32_t b, bool writable)
{
  state.size = sz;
  state.base = b;
  state.operands = new Operands[sz];
  state.opcode = new OPCODE_TYPE[sz + ILLEGAL_PC_THREAD_ADDR_OFFSET];
  state.executionFrequency = new executionFrequency_t[sz];
  if (writable) {
    state.invalidationInfo = new unsigned char[sz];
    for (unsigned i = 0; i != sz; ++i) {
      state.invalidationInfo[i] = INVALIDATE_NONE;
    }
  } else {
    state.invalidationInfo = 0;
  }
  std::memset(state.executionFrequency, 0,
              sizeof(state.executionFrequency[0]) * sz);
  initCache(Tracer::get().getTracingEnabled());
}

DecodeCache::~DecodeCache()
{
  delete[] state.operands;
  delete[] state.opcode;
  delete[] state.executionFrequency;
  delete[] state.invalidationInfo;
}

void DecodeCache::State::clearOpcode(uint32_t pc)
{
  opcode[pc] = getInstruction_DECODE(Tracer::get().getTracingEnabled());
}

void DecodeCache::State::
setOpcode(uint32_t pc, OPCODE_TYPE opc, unsigned size)
{
  opcode[pc] = opc;
  if (invalidationInfo) {
    if (invalidationInfo[pc] == INVALIDATE_NONE)
      invalidationInfo[pc] = INVALIDATE_CURRENT;
    assert((size % 2) == 0);
    for (unsigned i = 1; i < size / 2; i++) {
      invalidationInfo[pc + i] = INVALIDATE_CURRENT_AND_PREVIOUS;
    }
  }
}

void DecodeCache::State::
setOpcode(uint32_t pc, OPCODE_TYPE opc, Operands &ops, unsigned size)
{
  setOpcode(pc, opc, size);
  operands[pc] = ops;
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
