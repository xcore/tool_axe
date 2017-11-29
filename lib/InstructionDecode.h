// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _InstructionDecode_h_
#define _InstructionDecode_h_

#include "Config.h"
#include "Node.h"
#include "InstFunction.h"
#include <cstddef>

namespace axe {

struct Operands {
  union {
    uint32_t ops[3];
    uint8_t lops[6];
  };
};

} // End axe namespace

#endif //_InstructionDecode_h_
