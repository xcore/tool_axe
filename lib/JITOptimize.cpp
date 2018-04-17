// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "JITOptimize.h"
#include "Register.h"
#include "InstructionProperties.h"
#include "InstructionOpcode.h"
#include <algorithm>
#include <iostream>
#include <cassert>
#include <cstring>
#include <memory>
#include <array>
#include "stdio.h"

using namespace axe;

// baseReg + offsetImm + scale * offsetReg
class MemoryAccess {
  Register::Reg baseReg;
  uint32_t offsetImm;
  unsigned scale;
  Register::Reg offsetReg;
  unsigned size;
  bool isStore;
public:
  MemoryAccess() {}
  MemoryAccess(Register::Reg base, unsigned si, bool st) :
    baseReg(base),
    offsetImm(0),
    scale(0),
    size(si),
    isStore(st)
  {
  }

  MemoryAccess &addRegisterOffset(unsigned s, Register::Reg r) {
    assert(scale == 0);
    (void)scale;
    offsetReg = r;
    // Canonicalise.
    if (scale == 1 && offsetReg < baseReg)
      std::swap(offsetReg, baseReg);
    return *this;
  }
  MemoryAccess &addImmOffset(uint32_t offset) {
    offsetImm += offset;
    return *this;
  }
  Register::Reg getBaseReg() const { return baseReg; }
  unsigned getScale() const { return scale; }
  Register::Reg getOffsetReg() const { return baseReg; }
  uint32_t getOffsetImm() const { return offsetImm; }
  unsigned getSize() const { return size; }
  bool getIsStore() const { return isStore; }
};

class MemoryCheckCandidate {
  unsigned earliestIndex;
  unsigned instructionIndex;
  MemoryCheck check;
public:
  MemoryCheckCandidate(unsigned e, unsigned i, MemoryCheck c) :
    earliestIndex(e),
    instructionIndex(i),
    check(c) {}
  unsigned getEarliestIndex() const { return earliestIndex; }
  unsigned getInstructionIndex() const { return instructionIndex; }
  MemoryCheck getMemoryCheck() { return check; }
};

static bool
getInstructionMemoryAccess(InstructionOpcode opc, Operands ops,
                           MemoryAccess &access)
{
  bool isStore = false;
  switch (opc) {
  default: assert(0 && "Unexpected opcode");
  case LDWCP_ru6:
  case LDWCP_lru6:
    access = MemoryAccess(Register::CP, 4, false).addImmOffset(ops.ops[1]);
    return true;
  case LDWCPL_u10:
  case LDWCPL_lu10:
    access = MemoryAccess(Register::CP, 4, false).addImmOffset(ops.ops[0]);
    return true;
  case STWDP_ru6:
  case STWDP_lru6:
    isStore = true;
    // Fallthrough.
  case LDWDP_ru6:
  case LDWDP_lru6:
    access = MemoryAccess(Register::DP, 4, isStore).addImmOffset(ops.ops[1]);
    return true;
  case STWSP_ru6:
  case STWSP_lru6:
    isStore = true;
    // Fallthrough.
  case LDWSP_ru6:
  case LDWSP_lru6:
    access = MemoryAccess(Register::SP, 4, isStore).addImmOffset(ops.ops[1]);
    return true;
  case STDSP_l2rus:
    isStore = true;
  case LDDSP_l2rus:
    access = MemoryAccess(Register::SP, 8, isStore).addImmOffset(ops.ops[1] << 3);
  case ST8_l3r:
    isStore = true;
    // Fallthrough.
  case LD8U_3r:
    access = MemoryAccess(Register::Reg(ops.ops[1]), 1, isStore)
      .addRegisterOffset(1, Register::Reg(ops.ops[2]));
    return true;
  case ST16_l3r:
    isStore = true;
    // Fallthrough.
  case LD16S_3r:
    access = MemoryAccess(Register::Reg(ops.ops[1]), 2, isStore)
      .addRegisterOffset(2, Register::Reg(ops.ops[2]));
    return true;
  case STW_l3r:
    isStore = true;
    // Fallthrough.
  case LDW_3r:
    access = MemoryAccess(Register::Reg(ops.ops[1]), 4, isStore)
      .addRegisterOffset(4, Register::Reg(ops.ops[2]));
    return true;
  case STW_2rus:
    isStore = true;
    // Fallthrough.
  case LDW_2rus:
    access = MemoryAccess(Register::Reg(ops.ops[1]), 4, isStore)
      .addImmOffset(ops.ops[2]);
    return true;
  case STD_l3rus:
    isStore = true;
  case LDD_l3rus:
    access = MemoryAccess(Register::Reg(ops.ops[1]), 8, isStore)
      .addImmOffset(ops.ops[2] << 3);
    return true;
  case ENTSP_u6:
  case ENTSP_lu6:
    if (ops.ops[0] == 0)
      return false;
    access = MemoryAccess(Register::SP, 4, true);
    return true;
  case RETSP_u6:
  case RETSP_lu6:
  case RETSP_xs2a_u6:
  case RETSP_xs2a_lu6:
    if (ops.ops[0] == 0)
      return false;
    access = MemoryAccess(Register::SP, 4, false).addImmOffset(ops.ops[0]);
    return true;
  }
}

static bool isDef(OperandProperties::OpType type)
{
  return type == OperandProperties::out || type == OperandProperties::inout;
}

static Register::Reg
getRegister(const InstructionProperties &properties, const Operands &ops,
            unsigned i)
{
  if (i >= properties.getNumExplicitOperands())
    return properties.getImplicitOperand(i - properties.getNumExplicitOperands());
  return static_cast<Register::Reg>(ops.ops[i]);
}

static unsigned
getFlagsForCheck(const MemoryAccess &access)
{
  unsigned flags = 0;
  if (access.getSize() > 1)
    flags |= MemoryCheck::CheckAlignment;
  flags |= MemoryCheck::CheckAddress;
  if (access.getIsStore())
    flags |= MemoryCheck::CheckInvalidation;
  return flags;
}

struct MemoryCheckState {
  // Index of the first instruction where the value in the specified register
  // is available.
  std::array<unsigned, Register::NUM_REGISTERS> regDefs;
  std::array<unsigned char, Register::NUM_REGISTERS> minAlignment;

  MemoryCheckState() {
    regDefs.fill(0);
    minAlignment.fill(1);
  }

  void update(InstructionOpcode opc, const Operands &ops,
              unsigned nextOffset);
  void setMinAlignment(Register::Reg reg, unsigned char value) {
    minAlignment.at(reg) = value;
  }
  unsigned char getMinAlignment(Register::Reg reg) const {
    return minAlignment.at(reg);
  }
};

void MemoryCheckState::
update(InstructionOpcode opc, const Operands &ops, unsigned nextOffset)
{
  const InstructionProperties &properties = instructionProperties[opc];
  for (unsigned i = 0, e = properties.getNumOperands(); i != e; ++i) {
    if (!isDef(properties.getOperandType(i)))
      continue;
    Register::Reg reg = getRegister(properties, ops, i);
    regDefs.at(reg) = nextOffset;
    minAlignment.at(reg) = 1;
  }
}

void axe::
placeMemoryChecks(std::vector<InstructionOpcode> &opcode,
                  std::vector<Operands> &operands,
                  std::queue<std::pair<uint32_t,MemoryCheck>> &checks)
{
  MemoryCheckState state;

  std::vector<MemoryCheckCandidate> candidates;
  // Gather expressions for memory accesses.
  for (unsigned i = 0, e = opcode.size(); i != e; ++i) {
    InstructionOpcode opc = opcode[i];
    const Operands &ops = operands[i];
    InstructionProperties &properties = instructionProperties[opc];
    MemoryAccess access;
    if (properties.memCheckHoistingOptEnabled() &&
        getInstructionMemoryAccess(opc, ops, access)) {
      assert(access.getOffsetImm() % access.getSize() == 0);
      // Compute the first offset where all registers are available.
      unsigned first = state.regDefs.at(access.getBaseReg());
      if (access.getScale())
        first = std::min(first, state.regDefs.at(access.getOffsetReg()));
      unsigned flags = getFlagsForCheck(access);
      bool updateBaseRegAlign = false;
      if (flags & MemoryCheck::CheckAlignment) {
        assert(access.getScale() == 0 ||
               (access.getScale() % access.getSize()) == 0);
        assert((access.getOffsetImm() % access.getSize()) == 0);
        unsigned size = access.getSize();
        if ((state.getMinAlignment(access.getBaseReg()) % size) == 0) {
          flags &= ~MemoryCheck::CheckAlignment;
        } else {
          updateBaseRegAlign = true;
        }
      }
      const auto check =
        MemoryCheck(access.getSize(), access.getBaseReg(),
                        access.getScale(), access.getOffsetReg(),
                        access.getOffsetImm(), flags);
      candidates.push_back(MemoryCheckCandidate(first, i, check));
      if (updateBaseRegAlign) {
        state.setMinAlignment(access.getBaseReg(), access.getSize());
      }
    }
    // Update regDefs.
    state.update(opc, ops, i + 1);
  }
  if (candidates.empty())
    return;

  for (unsigned index = 0, size = candidates.size(); index < size; index++) {
    checks.push(std::make_pair(candidates[index].getInstructionIndex(),
                               candidates[index].getMemoryCheck()));
  }
}
