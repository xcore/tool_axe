// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PortInterface_h_
#define _PortInterface_h_

#include "Signal.h"

class PortInterface {
public:
  virtual void seePinsChange(const Signal &value, ticks_t time) = 0;
};

#endif // _PortInterface_h_
