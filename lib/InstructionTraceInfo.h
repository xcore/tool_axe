// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _InstructionTraceInfo_h_
#define _InstructionTraceInfo_h_

namespace axe {
  struct InstructionTraceInfo {
    const char *string;
  };
  
  extern const InstructionTraceInfo instructionTraceInfo[];
  
} // End axe namespace

#endif //_InstructionTraceInfo_h_
