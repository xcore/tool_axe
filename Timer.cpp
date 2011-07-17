// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Timer.h"
#include "ThreadState.h"

bool Timer::conditionMet(ticks_t time) const
{
  if (!after)
    return true;
  return ((int32_t)(time / CYCLES_PER_TICK) - (int32_t)data) > 0;
}

bool Timer::
setCondition(ThreadState &thread, Condition c, ticks_t time)
{
  updateOwner(thread);
  switch (c) {
  default: return false;
  case COND_FULL:
    after = false;
    break;
  case COND_AFTER:
    after = true;
    break;
  }
  return true;
}

bool Timer::
setData(ThreadState &thread, uint32_t d, ticks_t time)
{
  updateOwner(thread);
  data = d;
  return true;
}

Resource::ResOpResult Timer::
in(ThreadState &thread, ticks_t time, uint32_t &val)
{
  updateOwner(thread);
  if (!conditionMet(time)) {
    pausedIn = &thread;
    scheduleUpdate(getEarliestReadyTime(time));
    return DESCHEDULE;
  }
  val = (uint32_t)(time / CYCLES_PER_TICK);
  return CONTINUE;
}

ticks_t Timer::getEarliestReadyTime(ticks_t time) const
{
  if (conditionMet(time))
    return time;
  int32_t wait = (int32_t)(data + 1) - (int32_t)(time / CYCLES_PER_TICK);
  return time + wait * CYCLES_PER_TICK;
}

void Timer::run(ticks_t time)
{
  if (!conditionMet(time))
    return;
  if (eventsPermitted()) {
    event(time);
  }
  if (pausedIn) {
    pausedIn->time = time;
    pausedIn->schedule();
    pausedIn = 0;
  }
}

bool Timer::seeEventEnable(ticks_t time)
{
  if (conditionMet(time)) {
    event(time);
    return true;
  }
  scheduleUpdate(getEarliestReadyTime(time));
  return false;
}
