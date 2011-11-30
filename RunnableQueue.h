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
  class Sentinel : public Runnable {
  public:
    Sentinel() : Runnable() {}
    void run(ticks_t) {
      assert(0 && "Unimplemented\n");
    }
  };
  Sentinel head;
  bool contains(Runnable &thread) const
  {
    return thread.prev != 0;
  }
public:
  RunnableQueue() {}
  
  Runnable &front() const
  {
    return *head.next;
  }
  
  bool empty() const
  {
    return !head.next;
  }
  
  void remove(Runnable &thread)
  {
    assert(contains(thread));
    thread.prev->next = thread.next;
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
    Runnable *p = &head;
    while (p->next && time >= p->next->wakeUpTime)
      p = p->next;
    thread.prev = p;
    thread.next = p->next;
    if (p->next)
      p->next->prev = &thread;
    p->next = &thread;
  }
  
  void pop()
  {
    assert(head.next);
    remove(*head.next);
  }
};

#endif // _RunnableQueue_h_
