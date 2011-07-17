// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Runnable_h_
#define _Runnable_h_

#include "Config.h"
#include <cassert>

class Runnable {
public:
  enum RunnableType {
    THREAD,
    EVENTABLE_RESOURCE,
    SENTINEL
  };
private:
  RunnableType type;
public:
  Runnable *prev;
  Runnable *next;
  ticks_t wakeUpTime;

  RunnableType getType() const { return type; }
  virtual void run(ticks_t time) {
    assert(0 && "Unimplemented");
  }
  Runnable(RunnableType t) : type(t), prev(0), next(0) {}
};

#endif // _Runnable_h_
