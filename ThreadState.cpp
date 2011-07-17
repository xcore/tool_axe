// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "ThreadState.h"
#include "Core.h"
#include "Node.h"
#include "SystemState.h"
#include "Thread.h"
#include <iostream>

const char *registerNames[] = {
  "r0",
  "r1",
  "r2",
  "r3",
  "r4",
  "r5",
  "r6",
  "r7",
  "r8",
  "r9",
  "r10",
  "r11",
  "cp",
  "dp",
  "sp",
  "lr",
  "et",
  "ed",
  "kep",
  "ksp",
  "spc",
  "sed",
  "ssr"
};

unsigned ThreadState::getID() const
{
  return getRes().getNum();
}

void ThreadState::dump() const
{
  std::cout << std::hex;
  for (unsigned i = 0; i < NUM_REGISTERS; i++) {
    std::cout << getRegisterName(i) << ": 0x" << regs[i] << "\n";
  }
  std::cout << "pc: 0x" << getParent().targetPc(pc) << "\n";
  std::cout << std::dec;
}

void ThreadState::schedule()
{
  getParent().getParent()->getParent()->schedule(*this);
}

bool ThreadState::setSRSlowPath(sr_t enabled)
{
  if (enabled[EEBLE]) {
    EventableResourceList &EER = eventEnabledResources;
    for (EventableResourceList::iterator it = EER.begin(),
         end = EER.end(); it != end; ++it) {
      if ((*it)->seeOwnerEventEnable(time)) {
        return true;
      }
    }
  }
  if (enabled[IEBLE]) {
    EventableResourceList &IER = interruptEnabledResources;
    for (EventableResourceList::iterator it = IER.begin(),
         end = IER.end(); it != end; ++it) {
      if ((*it)->seeOwnerEventEnable(time)) {
        return true;
      }
    }
  }
  return false;
}

bool ThreadState::isExecuting() const
{
  return this == parent->getParent()->getParent()->getExecutingThread();
}
