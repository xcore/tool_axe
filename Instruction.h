// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Instruction_h_
#define _Instruction_h_

#include "Config.h"
#include <cstddef>

/// Below are all the instructions supported by the interpreter.
/// The instructions match those of the XCore except where noted below.
///
/// The operand for the relative branch instructions (bt, bf, bu, bl) is the
/// absolute translated pc value. There are two versions of each of these
/// instructions: one where the branch target is known to be legal
/// and one where it is known to be illegal.
///
/// There are two versions of the shift immediate instructions shr, ashr, shl,
/// one where the immediate is equal to 32 and one where it is less than 32.
///
/// The value for bitp operands is the decoded bitp value, except for
/// MKMSK_rus where the operand is the mask to use. The value for scaled
/// operands is the scaled value.
///
/// Finally there exist a number of additional pseduo instructions which don't
/// exist in the XCore instruction set which the interpreter uses.
enum InstructionOpcode {
#define EMIT_INSTRUCTION_LIST
#define DO_INSTRUCTION(inst) inst,
#include "InstructionGenOutput.inc"
#undef EMIT_INSTRUCTION_LIST
#undef DO_INSTRUCTION
};

#ifdef DIRECT_THREADED
typedef ptrdiff_t OPCODE_TYPE;
#else
typedef InstructionOpcode OPCODE_TYPE;
#endif

struct Operands {
  union {
    uint32_t ops[3];
    uint8_t lops[6];
  };
};

void
instructionDecode(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands);

#endif //_Instruction_h_
