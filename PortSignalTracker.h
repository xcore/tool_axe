// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PortSignalTracker_h_
#define _PortSignalTracker_h_

#include "PortInterface.h"
#include "Signal.h"

namespace axe {

class PortSignalTracker : public PortInterface {
  Signal signal;
public:
  PortSignalTracker();
  void seePinsChange(const Signal &s, ticks_t time) { signal = s; }
  const Signal &getSignal() const { return signal; }
};
  
} // End axe namespace

#endif //_PortSignalTracker_h_
