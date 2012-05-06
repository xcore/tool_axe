// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "RunnableQueue.h"

void RunnableQueue::push(Runnable &thread, ticks_t time)
{
  if (contains(thread)) {
    remove(thread);
  }
  thread.wakeUpTime = time;
  if (!head) {
    thread.next = 0;
    head = &thread;
  } else if (time < head->wakeUpTime) {
    head->prev = &thread;
    thread.next = head;
    head = &thread;
  } else {
    Runnable *p = head;
    while (p->next && time >= p->next->wakeUpTime)
      p = p->next;
    thread.prev = p;
    thread.next = p->next;
    if (p->next)
      p->next->prev = &thread;
    p->next = &thread;
  }
}
