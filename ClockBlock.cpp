// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "ClockBlock.h"
#include "Port.h"


ClockBlock::ClockBlock() :
  Resource(RES_TYPE_CLKBLK),
  source(0),
  readyIn(0),
  running(false)
{
}

void ClockBlock::updateAttachedPorts(ticks_t time)
{
  for (std::set<Port*>::iterator it = ports.begin(), e = ports.end();
       it != e; ++it) {
    (*it)->update(time);
  }
}

void ClockBlock::
seeChangeOnAttachedPorts(ticks_t time)
{
  if (!running)
    return;
  for (std::set<Port*>::iterator it = ports.begin(), e = ports.end();
       it != e; ++it) {
    (*it)->seeClockChange(time);
  }
}

void ClockBlock::
seeEdgeOnAttachedPorts(Edge::Type edgeType, ticks_t time) {
  for (std::set<Port*>::iterator it = ports.begin(), e = ports.end();
       it != e; ++it) {
    (*it)->seeEdge(edgeType, time);
  }
}
  
bool ClockBlock::
setCInUse(Thread &thread, bool val, ticks_t time)
{
  updateAttachedPorts(time);
  running = false;
  divide = 1;
  if (readyIn) {
    readyIn->deregisterAsReadyInOf(this);
    readyIn = 0;
    readyInValue.setValue(1);
  }
  setSourceRefClock(thread, time);
  setInUse(val);
  return true;
}

bool ClockBlock::setSource(Thread &thread, Port *p, ticks_t time)
{
  if (divide != 1)
    return false;
  p->update(time);
  setValue(p->getPinsValue(), time);
  if (source)
    source->deregisterAsSourceOf(this);
  source = p;
  source->registerAsSourceOf(this);
  return true;
}

void ClockBlock::setSourceRefClock(Thread &thread, ticks_t time)
{
  updateAttachedPorts(time);
  if (source) {
    source->deregisterAsSourceOf(this);
    source = 0;
  }
  Signal newValue = value;
  newValue.changeFrequency(time, getHalfPeriod());
  setValue(newValue, time);
}

bool ClockBlock::setData(Thread &thread, uint32_t newDivide, ticks_t time) {
  if (source)
    return false;
  newDivide = newDivide & 0xff;
  if (newDivide == 0)
    divide = 1;
  else
    divide = 2 * newDivide;
  Signal newValue = value;
  newValue.changeFrequency(time, getHalfPeriod());
  setValue(newValue, time);
  return true;
}

void ClockBlock::
setValue(const Signal &newValue, ticks_t time)
{
  if (newValue == value)
    return;
  if (isFixedFrequency() || newValue.isClock())
    updateAttachedPorts(time);
  uint32_t oldValue = value.getValue(time);
  value = newValue;
  if (!running)
    return;
  if (value.getValue(time) != oldValue) {
    seeEdgeOnAttachedPorts(oldValue == 0 ? Edge::RISING : Edge::FALLING, time);
  }
  if (isFixedFrequency()) {
    seeChangeOnAttachedPorts(time);
  }
}

Signal ClockBlock::getValue() const
{
  if (running)
    return value;
  return Signal(0);
}

bool ClockBlock::setReady(Thread &thread, Port *p, ticks_t time)
{
  updateAttachedPorts(time);
  p->update(time);
  if (p->getPortWidth() != 1)
    return false;
  if (readyIn)
    readyIn->deregisterAsReadyInOf(this);
  readyIn = p;
  readyIn->registerAsReadyInOf(this);
  readyInValue = readyIn->getPinsValue();
  seeChangeOnAttachedPorts(time);
  return true;
}

void ClockBlock::start(Thread &thread, ticks_t time)
{
  updateAttachedPorts(time);
  // Start the clock
  running = true;
  if (!source) {
    value.changeFrequency(time, 0, getHalfPeriod());
  }
  for (std::set<Port*>::iterator it = ports.begin(), e = ports.end();
       it != e; ++it) {
    // Update ports to current time
    (*it)->seeClockStart(time);
  }
}

void ClockBlock::stop(Thread &thread, ticks_t time)
{
  if (!running)
    return;
  updateAttachedPorts(time);
  running = false;
  seeChangeOnAttachedPorts(time);
}

void ClockBlock::setReadyInValue(Signal value, ticks_t time)
{
  updateAttachedPorts(time);
  readyInValue = value;
  seeChangeOnAttachedPorts(time);
}
