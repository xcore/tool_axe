// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Timeout_h
#define _Timeout_h

#include "Runnable.h"

namespace axe {

class Timeout : public Runnable {
public:
  virtual void run(ticks_t time);
};

} // End namespace axe

#endif //_Timeout_h_
