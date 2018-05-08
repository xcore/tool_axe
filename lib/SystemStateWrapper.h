// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _SystemStateWrapper_h
#define _SystemStateWrapper_h

#include "StopReason.h"
#include "Config.h"
#include <memory>

namespace axe {

class SystemState;
class Thread;

class SystemStateWrapper {
  std::unique_ptr<SystemState> system;
  Thread *lastBreakpointThread;
  int lastExitStatus;
  ticks_t lastStopTime;
public:
  SystemStateWrapper(std::unique_ptr<SystemState> s);
  SystemState *getSystemState() { return system.get(); }
  Thread *getThreadForLastBreakpoint() { return lastBreakpointThread; }
  int getLastExitStatus() const { return lastExitStatus; }
  StopReason::Type run(ticks_t numCycles);
};

} // End namespace axe

#endif // _SystemStateWrapper_h
