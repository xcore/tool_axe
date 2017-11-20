// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Synchroniser_h_

#define _Synchroniser_h_

#include "Resource.h"
#include <cassert>
#include <iostream>

namespace axe {

class Synchroniser : public Resource {
public:
  enum SyncResult {
    SYNC_CONTINUE,
    SYNC_DESCHEDULE,
    SYNC_KILL
  };
private:
  /// Number of threads.
  unsigned NumThreads;
  /// List of threads. The master thread is always at the first index.
  Thread *threads[NUM_THREADS];
  /// Number of paused threads
  unsigned NumPaused;
  bool join;
  /// Returns the time of thread with the latest time.
  ticks_t MaxThreadTime() const;
  SyncResult sync(Thread &thread, bool isMaster);
public:
  Synchroniser() : Resource(RES_TYPE_SYNC) {}
  
  bool alloc(Thread &master) override
  {
    assert(!isInUse() && "Trying to allocate in use synchroniser");
    setInUse(true);
    NumThreads = 1;
    threads[0] = &master;
    NumPaused = 0;
    join = false;
    return true;
  }
  
  void addChild(Thread &thread)
  {
    std::cout << "Adding child to Syncroniser\n";
    assert(NumThreads + 1 <= NUM_THREADS && "Too many threads");
    threads[NumThreads++] = &thread;
    NumPaused++;
  }
  
  bool free() override
  {
    setInUse(false);
    return true;
  }
  
  unsigned getNumThreads() const
  {
    return NumThreads;
  }
  
  unsigned getNumPaused() const
  {
    return NumPaused;
  }
  
  Thread &master()
  {
    return *threads[0];
  }
  
  SyncResult ssync(Thread &thread) { return sync(thread, false); }
  SyncResult msync(Thread &thread) { return sync(thread, true); }
  SyncResult mjoin(Thread &thread);

  void cancel() override;
};
  
} // End axe namespace

#endif // _Synchroniser_h_
