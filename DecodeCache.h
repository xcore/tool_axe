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
  enum {
    INVALIDATE_NONE,
    INVALIDATE_CURRENT,
    INVALIDATE_CURRENT_AND_PREVIOUS
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
    unsigned char *invalidationInfo;
    // Size in half words.
    uint32_t size;
    // Base in bytes
    uint32_t base;

    unsigned getRunJitAddr() const {
      return (size - 1) + RUN_JIT_ADDR_OFFSET;
    }
    
    unsigned getInterpretOneAddr() const {
      return (size - 1) + INTERPRET_ONE_ADDR_OFFSET;
    }
    
    unsigned getIllegalPCThreadAddr() const {
      return (size - 1) + ILLEGAL_PC_THREAD_ADDR_OFFSET;
    }

    bool contains(uint32_t address) const {
      return ((address - base) >> 1) < size;
    }

    uint32_t toPc(uint32_t address) const {
      return (address - base) >> 1;
    }
    
    uint32_t fromPc(uint32_t address) const {
      return (address << 1) + base;
    }

    unsigned char *getInvalidationInfo() {
      return invalidationInfo;
    }
    
    void setOpcode(uint32_t pc, OPCODE_TYPE opc, unsigned size);
    void setOpcode(uint32_t pc, OPCODE_TYPE opc, Operands &ops, unsigned size);
    void clearOpcode(uint32_t pc);
  };
private:
  State state;
  void initCache(bool tracing);
public:
  DecodeCache(uint32_t size, uint32_t base, bool writable);
  ~DecodeCache();
  State &getState() { return state; }
  const State &getState() const { return state; }
};


#endif // _DecodeCache_h_
