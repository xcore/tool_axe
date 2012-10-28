// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Lock_h_
#define _Lock_h_

#include <queue>
#include "Resource.h"

namespace axe {

class Lock : public Resource {
private:
  /// Is the lock currently held by a thread?
  bool held;
  /// Paused threads.
  std::queue<Thread *> threads;
public:
  Lock() : Resource(RES_TYPE_LOCK) {}

  bool alloc(Thread &master)
  {
    assert(!isInUse() && "Trying to allocate in use lock");
    setInUse(true);
    held = false;
    while (!threads.empty()) {
      threads.pop();
    }
    return true;
  }
  
  bool free()
  {
    // TODO what if there are still threads paused on the lock?
    setInUse(false);
    return true;
  }


  ResOpResult in(Thread &thread, ticks_t time, uint32_t &value);
  ResOpResult out(Thread &thread, uint32_t value, ticks_t time);
};
  
} // End axe namespace

#endif // _Lock_h_
