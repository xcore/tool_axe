// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _RunnableQueue_h_
#define _RunnableQueue_h_

#include "Runnable.h"
#include <cassert>
#include <queue>

namespace axe {

template<class T,
    class Container = std::vector<T>,
    class Compare = std::less<typename Container::value_type>> 

class MyQueue : public std::priority_queue<T, Container, Compare>
{
public:
    typedef typename
        std::priority_queue<
        T,
        Container,
        Compare>::container_type::const_iterator const_iterator;

    bool contains(const T&val) const
    {
        auto first = this->c.cbegin();
        auto last = this->c.cend();
        while (first!=last) {
            if (*first==val) return true;
            ++first;
        }
        return false;
    }
};


class CompareRunnable {
public:
  bool operator()(Runnable& r1, Runnable& r2)
  {
    if(r1.wakeUpTime < r2.wakeUpTime) return true;
    return false;
  }

  bool operator()(Runnable* r1, Runnable* r2)
  {
    if(r1->wakeUpTime < r2->wakeUpTime) return true;
    return false;
  }
};

class RunnableQueue {
private:
  typedef MyQueue<Runnable*, std::vector<Runnable*>, CompareRunnable> RunQueue;
  RunQueue pq;

  Runnable *head;
public:
  RunnableQueue() : head(0), pq(RunQueue()){}
  
  bool contains(Runnable &thread) const
  {
    return pq.contains(&thread);
  }
  
  Runnable &front() const
  {
    return *(pq.top());
  }
  
  bool empty() const
  {
    return pq.empty();
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
  
  const void pop()
  {
    assert(!empty());
    return pq.pop();
  }
};
  
} // End axe namespace

#endif // _RunnableQueue_h_
