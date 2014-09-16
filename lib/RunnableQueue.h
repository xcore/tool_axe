// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _RunnableQueue_h_
#define _RunnableQueue_h_

#include "Runnable.h"
#include <cassert>

namespace axe {

class RunnableQueue {
private:
  Runnable *head;
public:
  RunnableQueue() : head(0) {}
  
  bool contains(Runnable &thread) const
  {
    return thread.prev != 0 || &thread == head;
  }
  
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
  void push(Runnable &thread, ticks_t time);
  
  void pop()
  {
    assert(!empty());
    if (head->next) {
      head->next->prev = 0;
    }
    head = head->next;
  }
};
  
} // End axe namespace

#endif // _RunnableQueue_h_
