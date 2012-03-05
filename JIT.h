// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _JIT_h_
#define _JIT_h_

#include <stdint.h>
#include "JITInstructionFunction.h"

class Thread;
class Core;

namespace JIT {
  void compileBlock(Core &c, uint32_t address);
  bool invalidate(Core &c, uint32_t shiftedAddress);
};

#endif // _JIT_h_
