// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _StopReason_h_
#define _StopReason_h_

#include <cassert>
#include "Config.h"

namespace axe {

class Thread;

class StopReason {
public:
  enum Type {
    BREAKPOINT,
    WATCHPOINT,
    TIMEOUT,
    EXIT,
    NO_RUNNABLE_THREADS
  };
private:
  Type type;
  ticks_t time;
  union {
    Thread *thread;
    int status;
  };
  StopReason(Type ty, ticks_t t) : type(ty), time(t) {}
public:
  Type getType() const { return type; }
  ticks_t getTime() const { return time; }
  Thread *getThread() const {
    assert(type == BREAKPOINT || type == WATCHPOINT);
    return thread;
  }
  int getStatus() const {
    assert(type == EXIT);
    return status;
  }

  static StopReason getTimeout(ticks_t time);
  static StopReason getBreakpoint(ticks_t time, Thread &thread);
  static StopReason getWatchpoint(ticks_t time, Thread &thread);
  static StopReason getExit(ticks_t time, int status);
  static StopReason getNoRunnableThreads(ticks_t time);
};

} // End axe namespace

#endif // _StopReason_h_
