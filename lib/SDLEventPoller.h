// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _SDLEventPoller_h_
#define _SDLEventPoller_h_

#include "Runnable.h"
#include <stdint.h>

namespace axe {

class RunnableQueue;

class SDLEventPoller : public Runnable {
  RunnableQueue &scheduler;
  uint32_t lastPollEvent;
public:
  SDLEventPoller(RunnableQueue &scheduler);
  
  void run(ticks_t time) override;
};

}

#endif //_SDLEventPoller_h_
