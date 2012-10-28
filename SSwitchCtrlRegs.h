// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _SSwitchCtrlRegs_h_
#define _SSwitchCtrlRegs_h_

#include <stdint.h>
#include <vector>

namespace axe {

class Node;

class SSwitchCtrlRegs {
private:
  Node *node;
  uint32_t scratchReg;
  std::vector<uint8_t> regFlags;
  enum RegisterFlags {
    REG_READ = 1,
    REG_WRITE = 1 << 1,
    REG_RW = REG_READ | REG_WRITE
  };
  void initReg(unsigned num, uint8_t flags);
public:
  SSwitchCtrlRegs(Node *n);
  void initRegisters();
  bool read(uint16_t num, uint32_t &result);
  bool write(uint16_t num, uint32_t value);
};
  
} // End axe namespace

#endif //_SSwitchCtrlRegs_h_
