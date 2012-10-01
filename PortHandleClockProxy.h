// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PortHandleClockProxy_h_
#define _PortHandleClockProxy_h_

#include "Signal.h"
#include "PortHandleClockMixin.h"

class PortHandleClockProxy : public PortHandleClockMixin<PortHandleClockProxy> {
  PortInterface &next;
public:
  PortHandleClockProxy(RunnableQueue &scheduler, PortInterface &next);
  void seePinsValueChange(uint32_t value, ticks_t time) {
    next.seePinsChange(Signal(value), time);
  }
};

#endif // _PortHandleClockProxy_h_
