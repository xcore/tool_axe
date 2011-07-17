// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Synchroniser.h"
#include "Core.h"

ticks_t Synchroniser::MaxThreadTime() const
{
  ticks_t time = threads[0]->time;
  for (unsigned i = 1; i < NumThreads; i++) {
    if (threads[i]->time > time)
      time = threads[i]->time;
  }
  return time;
}

Synchroniser::SyncResult Synchroniser::
sync(ThreadState &thread)
{
  if (getNumPaused() + 1 < getNumThreads()) {
    // Pause the current thread
    NumPaused++;
    return SYNC_DESCHEDULE;
  }
  // Wakeup / kill threads
  ticks_t newTime = MaxThreadTime();
  NumPaused = 0;
  if (!join) {
    for (unsigned i = 0; i < NumThreads; i++) {
      threads[i]->time = newTime;
      if (threads[i] != &thread) {
        threads[i]->pc++;
        threads[i]->schedule();
      }
    }
    return SYNC_CONTINUE;
  }
  SyncResult result;
  threads[0]->time = newTime;
  if (&thread == &master()) {
    result = SYNC_CONTINUE;
    for (unsigned i = 1; i < NumThreads; i++) {
      threads[i]->getRes().free();
    }
  } else {
    master().schedule();
    result = SYNC_KILL;    
    for (unsigned i = 1; i < NumThreads; i++) {
      if (threads[i] != &thread) {
        threads[i]->getRes().free();
      }
    }
  }
  join = false;
  NumThreads = 1;
  return result;
}

Synchroniser::SyncResult Synchroniser::
mjoin(ThreadState &thread)
{
  join = true;
  return sync(thread);
}

void Synchroniser::cancel()
{
  NumPaused--;
}
