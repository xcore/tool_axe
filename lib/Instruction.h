// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Instruction_h_
#define _Instruction_h_

#include "Config.h"
#include "Node.h"
#include "InstFunction.h"
#include <cstddef>

namespace axe {

class Core;
class Thread;
enum InstructionOpcode : short;

typedef InstFunction_t OPCODE_TYPE;

struct Operands {
  uint32_t ops[6];
};

void instructionDecode(const Core &tile, uint32_t addr,
                       InstructionOpcode &opcode, Operands &operands,
                       bool ignoreBreakpoints = false);

void
instructionDecode(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands, Node::Type type);

void
instructionTransform(InstructionOpcode &opc, Operands &operands,
                     const Core &tile, uint32_t address, bool isDualIssue);
  
} // End axe namespace

#endif //_Instruction_h_
