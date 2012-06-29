// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _DecodeCache_h_
#define _DecodeCache_h_

#include <stdint.h>
#include <climits>
#include "Instruction.h"

struct Operands;

class DecodeCache {
public:
  typedef int executionFrequency_t;
  static const executionFrequency_t MIN_EXECUTION_FREQUENCY = INT_MIN;
  enum {
    ILLEGAL_ADDR_OFFSET = 1,
    RUN_JIT_ADDR_OFFSET,
    INTERPRET_ONE_ADDR_OFFSET,
    ILLEGAL_PC_THREAD_ADDR_OFFSET
  };
  struct State {
    // The opcode cache is bigger than the memory size. We place an ILLEGAL_PC
    // pseudo instruction just past the end of memory. This saves
    // us from having to check for illegal pc values when incrementing the pc from
    // the previous instruction. Addition pseudo instructions come after this and
    // are use for communicating illegal states.
    OPCODE_TYPE *opcode;
    Operands *operands;
    executionFrequency_t *executionFrequency;
    // TODO store as size in half words?
    uint32_t size;
    uint32_t base;

    void clearOpcode(uint32_t pc);

    unsigned getRunJitAddr() const {
      return (size / 2 - 1) + RUN_JIT_ADDR_OFFSET;
    }
    
    unsigned getInterpretOneAddr() const {
      return (size / 2 - 1) + INTERPRET_ONE_ADDR_OFFSET;
    }
    
    unsigned getIllegalPCThreadAddr() const {
      return (size / 2 - 1) + ILLEGAL_PC_THREAD_ADDR_OFFSET;
    }
  };
private:
  State state;
  void initCache(bool tracing);
public:
  DecodeCache(uint32_t size, uint32_t base);
  ~DecodeCache();
  State &getState() { return state; }
  const State &getState() const { return state; }
};


#endif // _DecodeCache_h_
