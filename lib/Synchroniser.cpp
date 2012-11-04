// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Synchroniser.h"
#include "Core.h"

using namespace axe;

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
sync(Thread &thread, bool isMaster)
{
  if (getNumPaused() + 1 < getNumThreads()) {
    // Pause the current thread
    NumPaused++;
    if (!isMaster)
      thread.setSSync(true);
    return SYNC_DESCHEDULE;
  }
  // Wakeup / kill threads
  ticks_t newTime = MaxThreadTime();
  NumPaused = 0;
  if (!join) {
    for (unsigned i = 0; i < NumThreads; i++) {
      threads[i]->time = newTime;
      if (threads[i] != &thread) {
        if (i > 0)
          threads[i]->setSSync(false);
        threads[i]->pc++;
        threads[i]->schedule();
      }
    }
    return SYNC_CONTINUE;
  }
  SyncResult result;
  threads[0]->time = newTime;
  if (isMaster) {
    result = SYNC_CONTINUE;
    for (unsigned i = 1; i < NumThreads; i++) {
      threads[i]->free();
    }
  } else {
    master().schedule();
    result = SYNC_KILL;    
    for (unsigned i = 1; i < NumThreads; i++) {
      if (threads[i] != &thread) {
        threads[i]->free();
      }
    }
  }
  join = false;
  NumThreads = 1;
  return result;
}

Synchroniser::SyncResult Synchroniser::
mjoin(Thread &thread)
{
  join = true;
  return sync(thread, true);
}

void Synchroniser::cancel()
{
  NumPaused--;
}
