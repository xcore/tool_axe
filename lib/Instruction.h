// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Instruction_h_
#define _Instruction_h_

#include "Config.h"
#include "JITInstructionFunction.h"
#include <cstddef>

namespace axe {

class Core;
class Thread;
enum InstructionOpcode : short;

typedef JITInstructionFunction_t OPCODE_TYPE;

struct Operands {
  union {
    uint32_t ops[3];
    uint8_t lops[6];
  };
};

void instructionDecode(Core &core, uint32_t addr, InstructionOpcode &opcode,
                       Operands &operands, bool ignoreBreakpoints = false);

void
instructionDecode(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands);

void
instructionTransform(InstructionOpcode &opc, Operands &operands,
                     const Core &core, uint32_t address);
  
} // End axe namespace

#endif //_Instruction_h_
