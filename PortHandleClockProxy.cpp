// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PortHandleClockProxy.h"
#include "Signal.h"
#include "RunnableQueue.h"

PortHandleClockProxy::
PortHandleClockProxy(RunnableQueue &s, PortInterface &p) :
  scheduler(s),
  next(p),
  currentValue(0),
  currentSignal(0)
{
}

void PortHandleClockProxy::seePinsChange(const Signal &newSignal, ticks_t time)
{
  uint32_t newValue = newSignal.getValue(time);
  if (newValue != currentValue) {
    next.seePinsChange(Signal(newValue), time);
    currentValue = newValue;
  }
  currentSignal = newSignal;
  if (currentSignal.isClock()) {
    scheduler.push(*this, currentSignal.getNextEdge(time).time);
  }
}

void PortHandleClockProxy::run(ticks_t time)
{
  uint32_t newValue = currentSignal.getValue(time);
  if (newValue == currentValue)
    return;
  next.seePinsChange(Signal(newValue), time);
  assert(currentSignal.isClock());
  scheduler.push(*this, currentSignal.getNextEdge(time).time);
}
