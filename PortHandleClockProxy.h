// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PortHandleClockProxy_h_
#define _PortHandleClockProxy_h_

#include "PortInterface.h"
#include "Runnable.h"
#include "Signal.h"

class RunnableQueue;

class PortHandleClockProxy : public PortInterface, public Runnable {
  RunnableQueue &scheduler;
  PortInterface &next;
  uint32_t currentValue;
  Signal currentSignal;
public:
  PortHandleClockProxy(RunnableQueue &scheduler, PortInterface &next);
  void seePinsChange(const Signal &value, ticks_t time);
  void run(ticks_t time);
};

#endif // _PortHandleClockProxy_h_
