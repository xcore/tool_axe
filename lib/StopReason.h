// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _StopReason_h_
#define _StopReason_h_

#include <cassert>

namespace axe {

class Thread;

class StopReason {
public:
  enum Type {
    BREAKPOINT,
    TIMEOUT,
    EXIT,
    NO_RUNNABLE_THREADS
  };
private:
  Type type;
  union {
    Thread *thread;
    int status;
  };
  StopReason(Type t) : type(t) {}
public:
  Type getType() const { return type; }
  Thread *getThread() const {
    assert(type == BREAKPOINT);
    return thread;
  }
  int getStatus() const {
    assert(type == EXIT);
    return status;
  }

  static StopReason getTimeout();
  static StopReason getBreakpoint(Thread &thread);
  static StopReason getExit(int status);
  static StopReason getNoRunnableThreads();
};

} // End axe namespace

#endif // _StopReason_h_
