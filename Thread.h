// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Thread_h_
#define _Thread_h_

#include "Resource.h"
#include "ThreadState.h"

class Thread : public Resource {
private:
  ThreadState state;
public:
  Thread() : Resource(RES_TYPE_THREAD), state(*this) {
    setInUse(false);
  }
  const ThreadState &getState() const { return state; }
  ThreadState &getState() { return state; }

  bool alloc(ThreadState &CurrentThread)
  {
    alloc(CurrentThread.time);
    return true;
  }
  
  void alloc(ticks_t t)
  {
    assert(!isInUse() && "Trying to allocate in use thread");
    setInUse(true);
    state.alloc(t);
  }

  bool free()
  {
    setInUse(false);
    return true;
  }
};

#endif //_Thread_h_
