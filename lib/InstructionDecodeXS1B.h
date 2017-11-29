// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _InstructionDecodeXS1B_h_
#define _InstructionDecodeXS1B_h_

#include "InstructionDecode.h"

namespace axe {

void
instructionDecodeXS1B(uint16_t low, uint16_t high, bool highValid,
                      InstructionOpcode &opcode, Operands &operands);

} // End axe namespace

#endif //_InstructionDecodeXS1B_h_
