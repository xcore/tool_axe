// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Runnable_h_
#define _Runnable_h_

#include "Config.h"
#include <cassert>

namespace axe {

class Runnable {
protected:
  ~Runnable() = default;
public:
  Runnable *prev;
  Runnable *next;
  ticks_t wakeUpTime;

  virtual void run(ticks_t time) = 0;
  Runnable() : prev(0), next(0) {}
};
  
} // End axe namespace

#endif // _Runnable_h_
