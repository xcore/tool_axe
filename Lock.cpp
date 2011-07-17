// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Resource.h"
#include "Core.h"

Resource::ResOpResult Lock::
out(ThreadState &thread, uint32_t value, ticks_t time)
{
  // Unpause a thread.
  if (!threads.empty()) {
    ThreadState *next = threads.front();
    threads.pop();
    if (time > next->time)
      next->time = time;
    next->pc++;
    next->schedule();
  } else {
    held = false;
  }
  return CONTINUE;
}

Resource::ResOpResult Lock::
in(ThreadState &thread, ticks_t time, uint32_t &value)
{
  if (!held) {
    held = true;
    value = getID();
    return CONTINUE;
  }
  threads.push(&thread);
  return DESCHEDULE;
}
