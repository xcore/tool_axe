// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "SystemStateWrapper.h"
#include "SystemState.h"

using namespace axe;

SystemStateWrapper::
SystemStateWrapper(std::unique_ptr<SystemState> s) :
  system(std::move(s)), lastBreakpointThread(0), lastExitStatus(0),
  lastStopTime(0) {
  
}

StopReason::Type SystemStateWrapper::run(ticks_t numCycles) {
  if (numCycles == 0) {
    system->clearTimeout();
  } else {
    system->setTimeout(lastStopTime + numCycles);
  }
  StopReason stopReason = system->run();
  lastStopTime = stopReason.getTime();
  switch (stopReason.getType()) {
  default:
    break;
  case StopReason::BREAKPOINT:
    lastBreakpointThread = stopReason.getThread();
    break;
  case StopReason::EXIT:
    lastExitStatus = stopReason.getStatus();
    break;
  }
  return stopReason.getType();
}
