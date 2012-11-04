// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PortHandleClockMixin_h_
#define _PortHandleClockMixin_h_

#include "PortInterface.h"
#include "Runnable.h"
#include "RunnableQueue.h"
#include "Signal.h"

namespace axe {

class RunnableQueue;

/// Mixin which handles calls to seePinsChange() and calls seePinsValueChange()
/// whenever the value on the pins changes. Makes use of the curiously recurring
/// template pattern to avoid dynamic dispatch. To make use this class inherit
/// from it using the Derivied class as the template parameter.
template <class Derived>
class PortHandleClockMixin : public PortInterface, public Runnable {
  RunnableQueue &scheduler;
  uint32_t currentValue;
  Signal currentSignal;
public:
  PortHandleClockMixin(RunnableQueue &s) :
  scheduler(s), currentValue(0), currentSignal(0) {}
  void seePinsChange(const Signal &value, ticks_t time);
  void run(ticks_t time);
};


template <class Derived>
void PortHandleClockMixin<Derived>::
seePinsChange(const Signal &newSignal, ticks_t time)
{
  bool wasClock = currentSignal.isClock();
  currentSignal = newSignal;
  uint32_t newValue = currentSignal.getValue(time);
  if (newValue != currentValue) {
    currentValue = newValue;
    static_cast<Derived*>(this)->seePinsValueChange(currentValue, time);
  }
  if (currentSignal.isClock()) {
    scheduler.push(*this, currentSignal.getNextEdge(time).time);
  } else if (wasClock) {
    scheduler.remove(*this);
  }
}

template <class Derived>
void PortHandleClockMixin<Derived>::run(ticks_t time)
{
  assert(currentSignal.isClock());
  currentValue = !currentValue;
  static_cast<Derived*>(this)->seePinsValueChange(currentValue, time);
  scheduler.push(*this, time += currentSignal.getHalfPeriod());
}
  
} // End axe namespace

#endif //_PortHandleClockMixin_h_
