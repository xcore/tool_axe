// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _SystemState_h_
#define _SystemState_h_

#include <vector>
#include <memory>
#include "ThreadState.h"
#include "RunnableQueue.h"

class Node;
class ChanEndpoint;

class SystemState {
  std::vector<Node*> nodes;
  RunnableQueue scheduler;
  /// The currently executing thread.
  ThreadState *currentThread;
  PendingEvent pendingEvent;

  void handleNonThreads() {
    assert(currentThread == 0);
    while (!scheduler.empty() &&
           scheduler.front().getType() != Runnable::THREAD) {
      Runnable &runnable = scheduler.front();
      scheduler.pop();
      runnable.run(runnable.wakeUpTime);
    }
  }

  void completeEvent(ThreadState &t, EventableResource &res, bool interrupt);

public:
  SystemState() : currentThread(0) {
    pendingEvent.set = false;
  }
  ~SystemState();
  void setCurrentThread(ThreadState &thread) { currentThread = &thread; }
  void addNode(std::auto_ptr<Node> n);
  const std::vector<Node*> &getNodes() const { return nodes; }
  
  bool hasTimeSliceExpired(ticks_t time) const {
    if (scheduler.empty())
      return false;
    return time > scheduler.front().wakeUpTime;
  }

  void setExecutingThread(ThreadState &thread) {
    currentThread = &thread;
  }

  ThreadState *getExecutingThread() {
    return currentThread;
  }
  
  /// Schedule a thread.
  void schedule(ThreadState &thread) {
    thread.waiting() = false;
    thread.pausedOn = 0;
    scheduler.push(thread, thread.time);
  }
  
  void scheduleOther(Runnable &runnable, ticks_t time) {
    scheduler.push(runnable, time);
  }
  
  ThreadState *deschedule(ThreadState &current);

  ThreadState *deschedule(ThreadState &current, Resource *res)
  {
    current.pausedOn = res;
    return deschedule(current);
  }

  ThreadState *next(ThreadState &current, ticks_t time) {
    assert(&current == currentThread);
    assert(!current.waiting());
    if (!hasTimeSliceExpired(time))
      return &current;
    scheduler.push(current, time);
    currentThread = 0;
    handleNonThreads();
    ThreadState &next = static_cast<ThreadState&>(scheduler.front());
    currentThread = &next;
    scheduler.pop();
    return &next;
  }
  
  /// Take an event on a thread. The thread must not be the current thread.
  void takeEvent(ThreadState &thread, EventableResource &res, bool interrupt)
  {
    if (thread.waiting()) {
      if (thread.pausedOn) {
        thread.pausedOn->cancel();
      }
      schedule(thread);
    }
    completeEvent(thread, res, interrupt);
  }

  /// Take an event on the current thread.
  /// \param CycleThread Whether to cycle the running thread after the
  ///        event is taken.
  /// \return The new time and pc.
  ThreadState *takeEvent(ThreadState &current, bool cycleThread = true)
  {
    current.time = std::max(current.time, pendingEvent.time);
    // TODO this is probably the wrong place for this.
    current.waiting() = false;
    completeEvent(current, *pendingEvent.res, pendingEvent.interrupt);
    pendingEvent.set = false;
    // This is to ensure one thread can't stave the others.
    if (cycleThread) {
      return next(current, current.time);
    }
    return &current;
  }
  
  /// Sets a pending event on the current thread.
  void setPendingEvent(EventableResource &res, ticks_t time, bool interrupt)
  {
    if (pendingEvent.set && pendingEvent.time <= time)
      return;
    pendingEvent.set = true;
    pendingEvent.res = &res;
    pendingEvent.interrupt = interrupt;
    pendingEvent.time = time;
  }
  
  bool hasPendingEvent() const {
    return pendingEvent.set;
  }

  ChanEndpoint *getChanendDest(ResourceID ID);
};

#endif // _SystemState_h_
