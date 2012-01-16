// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _InstructionProperties_h_
#define _InstructionProperties_h_

struct InstructionProperties {
  const char *function;
  unsigned char size;
  unsigned char numExplicitOperands;
};

extern InstructionProperties instructionProperties[];

#endif //_InstructionProperties_h_
