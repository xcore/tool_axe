// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _InstructionProperties_h_
#define _InstructionProperties_h_

#include "Register.h"

namespace axe {

namespace OperandProperties {
  enum OpType {
    in,
    out,
    inout,
    imm
  };
}

struct InstructionProperties {
  enum Flags {
    MAY_BRANCH = 1 << 0,
    MAY_YIELD = 1 << 1,
    MAY_DESCHEDULE = 1 << 2,
    MAY_END_TRACE = 1 << 3,
    MEM_CHECK_OPT_ENABLED = 1 << 4
  };
  const char *function;
  OperandProperties::OpType *ops;
  Register::Reg *implicitOps;
  unsigned char size;
  unsigned char numOperands;
  unsigned char numImplicitOperands;
  unsigned char flags;
  bool mayBranch() const { return flags & MAY_BRANCH; }
  bool mayYield() const { return flags & MAY_YIELD; }
  bool mayDeschedule() const { return flags & MAY_DESCHEDULE; }
  bool mayEndTrace() const { return flags & MAY_END_TRACE; }
  bool memCheckHoistingOptEnabled() const {
    return flags & MEM_CHECK_OPT_ENABLED;
  }
  unsigned getNumOperands() const { return numOperands; }
  unsigned getNumExplicitOperands() const {
    return numOperands - numImplicitOperands;
  }
  OperandProperties::OpType getOperandType(unsigned i) const {
    return ops[i];
  }
  Register::Reg getImplicitOperand(unsigned i) const {
    return implicitOps[i];
  }
};

extern InstructionProperties instructionProperties[];
  
} // End axe namespace

#endif //_InstructionProperties_h_
