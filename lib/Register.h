// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Register_h_
#define _Register_h_

namespace axe {

namespace Register {
  enum Reg {
    R0,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,
    R8,
    R9,
    R10,
    R11,
    CP,
    DP,
    SP,
    LR,
    ET,
    ED,
    KEP,
    KSP,
    SPC,
    SED,
    SSR,
    NUM_REGISTERS
  };
}

extern const char *registerNames[];

inline const char *getRegisterName(unsigned RegNum) {
  if (RegNum < Register::NUM_REGISTERS) {
    return registerNames[RegNum];
  }
  return "?";
}

} // End axe namespace

#endif //_Register_h_
