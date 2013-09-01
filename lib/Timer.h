// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Timer_h_
#define _Timer_h_

#include "Resource.h"

namespace axe {

class Timer : public EventableResource {
private:
  bool after;
  uint32_t data;
  /// Thread paused on an input instruction.
  Thread *pausedIn;

  /// Return whether the condition is met for the specified time.
  bool conditionMet(ticks_t time) const;
public:
  Timer() :
    EventableResource(RES_TYPE_TIMER),
    pausedIn(0) {}

  bool alloc(Thread &t) override
  {
    assert(!isInUse() && "Trying to allocate in use timer");
    after = false;
    data = 0;
    eventableSetInUseOn(t);
    return true;
  }

  bool free() override
  {
    eventableSetInUseOff();
    return true;
  }

  bool setCondition(Thread &thread, Condition c, ticks_t time) override;
  bool setData(Thread &thread, uint32_t d, ticks_t time) override;

  ResOpResult in(Thread &thread, ticks_t time, uint32_t &val) override;

  /// Returns the earliest time at which the timer will become ready.
  ticks_t getEarliestReadyTime(ticks_t time) const;
  
  void run(ticks_t time) override;

protected:
  bool seeEventEnable(ticks_t time) override;
};
  
} // End axe namespace

#endif // _Timer_h_
