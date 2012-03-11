// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _RunnableQueue_h_
#define _RunnableQueue_h_

#include "Runnable.h"
#include <cassert>

class RunnableQueue {
private:
  Runnable *head;
  bool contains(Runnable &thread) const
  {
    return thread.prev != 0 || &thread == head;
  }
public:
  RunnableQueue() : head(0) {}
  
  Runnable &front() const
  {
    return *head;
  }
  
  bool empty() const
  {
    return !head;
  }
  
  void remove(Runnable &thread)
  {
    assert(contains(thread));
    if (&thread == head) {
      head = thread.next;
    } else {
      thread.prev->next = thread.next;
    }
    if (thread.next)
      thread.next->prev = thread.prev;
    thread.prev = 0;
  }
  
  // Insert a thread into the queue.
  void push(Runnable &thread, ticks_t time)
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
  
  void pop()
  {
    assert(!empty());
    if (head->next) {
      head->next->prev = 0;
    }
    head = head->next;
  }
};

#endif // _RunnableQueue_h_
