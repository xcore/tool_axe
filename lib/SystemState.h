// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _SystemState_h_
#define _SystemState_h_

#include <vector>
#include <memory>
#include "Thread.h"
#include "JIT.h"
#include "RunnableQueue.h"
#include "Timeout.h"

namespace axe {

class Node;
class ChanEndpoint;
class DecodeCache;
class SyscallHandler;
class Tracer;

class ExitException {
  unsigned status;
public:
  ExitException(unsigned s) : status(s) {}
  unsigned getStatus() const { return status; }
};

class TimeoutException {
  ticks_t time;
public:
  TimeoutException(ticks_t t) : time(t) {}
  ticks_t getTime() const { return time; }
};

class SystemState {
  std::vector<Node*> nodes;
  RunnableQueue scheduler;
  /// The currently executing runnable.
  Runnable *currentRunnable;
  PendingEvent pendingEvent;
  Timeout timeoutRunnable;

  uint8_t *rom;
  std::auto_ptr<DecodeCache> romDecodeCache;

  JIT jit;
  SyscallHandler *syscallHandler;
  Tracer *tracer;

  void completeEvent(Thread &t, EventableResource &res, bool interrupt);

public:
  typedef std::vector<Node*>::iterator node_iterator;
  typedef std::vector<Node*>::const_iterator const_node_iterator;
  SystemState(bool tracing);
  ~SystemState();
  void finalize();
  RunnableQueue &getScheduler() { return scheduler; }
  void addNode(std::auto_ptr<Node> n);

  Runnable *getExecutingRunnable() {
    return currentRunnable;
  }

  void setTimeout(ticks_t time);

  int run();

  /// Schedule a thread.
  void schedule(Thread &thread) {
    thread.waiting() = false;
    thread.pausedOn = 0;
    scheduler.push(thread, thread.time);
  }
  
  void scheduleOther(Runnable &runnable, ticks_t time) {
    scheduler.push(runnable, time);
  }
  
  /// Take an event on a thread. The thread must not be the current thread.
  void takeEvent(Thread &thread, EventableResource &res, bool interrupt)
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
  void takeEvent(Thread &current)
  {
    current.time = std::max(current.time, pendingEvent.time);
    // TODO this is probably the wrong place for this.
    current.waiting() = false;
    completeEvent(current, *pendingEvent.res, pendingEvent.interrupt);
    pendingEvent.set = false;
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

  void setRom(const uint8_t *data, uint32_t romSize,
              uint32_t romBase = 0xffffc000);
  const DecodeCache::State &getRomDecodeCache() const {
    return romDecodeCache->getState();
  }

  JIT &getJIT() { return jit; }
  SyscallHandler &getSyscallHandler() { return *syscallHandler; }
  Tracer &getTracer() { return *tracer; }

  node_iterator node_begin() { return nodes.begin(); }
  node_iterator node_end() { return nodes.end(); }
  const_node_iterator node_begin() const { return nodes.begin(); }
  const_node_iterator node_end() const { return nodes.end(); }
};
  
} // End axe namespace

#endif // _SystemState_h_
