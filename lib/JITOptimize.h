// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _JITOptimize_h_
#define _JITOptimize_h_

#include <vector>
#include <queue>
#include <utility>
#include "Instruction.h"
#include "Register.h"

namespace axe {

// Let x = baseReg + scale + offsetReg + offsetImm
class MemoryCheck {
public:
  enum Type {
    CheckAlignment = 1 << 0,
    CheckAddress = 1 << 1,
    CheckInvalidation = 1 << 2,
  };
private:
  unsigned size;
  Register::Reg baseReg;
  unsigned scale;
  Register::Reg offsetReg;
  uint32_t offsetImm;
  /// Bitwise OR of MemoryCheck::Type.
  unsigned flags;
public:
  MemoryCheck(unsigned Size, Register::Reg base, unsigned s,
              Register::Reg offset, uint32_t offsetI, unsigned f) :
  size(Size),
  baseReg(base),
  scale(s),
  offsetReg(offset),
  offsetImm(offsetI),
  flags(f) {}

  unsigned getSize() const { return size; }
  Register::Reg getBaseReg() const { return baseReg; }
  unsigned getScale() const { return scale; }
  Register::Reg getOffsetReg() const { return offsetReg; }
  uint32_t getOffsetImm() const { return offsetImm; }
  unsigned getFlags() const { return flags; }
};

void placeMemoryChecks(std::vector<InstructionOpcode> &opcode,
                       std::vector<Operands> &operands,
                       std::queue<std::pair<uint32_t,MemoryCheck*>> &checks);
  
} // End axe namespace

#endif // _JITOptimize_h_
