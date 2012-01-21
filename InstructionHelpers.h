// Copyright (c) 2011-12, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _InstructionHelpers_h_
#define _InstructionHelpers_h_

#include <stdint.h>

class Thread;

uint32_t exception(Thread &t, uint32_t pc, int et, uint32_t ed);

#endif // _InstructionHelpers_h_
