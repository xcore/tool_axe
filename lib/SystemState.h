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
#include "SymbolInfo.h"

namespace axe {

class Node;
class ChanEndpoint;
class DecodeCache;
class Tracer;
class StopReason;
#if AXE_ENABLE_SDL
class SDLEventPoller;
#endif

class StopException {
  ticks_t time;
protected:
  StopException(ticks_t t) : time(t) {}
public:
  ticks_t getTime() const { return time; }
};

class ExitException : public StopException {
  unsigned status;
public:
  ExitException(ticks_t time, unsigned s) :
    StopException(time), status(s) {}
  unsigned getStatus() const { return status; }
};

class TimeoutException : public StopException {
public:
  TimeoutException(ticks_t time) : StopException(time) {}
};

class BreakpointException : public StopException {
  Thread &thread;
public:
  BreakpointException(ticks_t time, Thread &t) :
    StopException(time), thread(t) {}
  Thread &getThread() const { return thread; }
};

class SystemState {
  std::vector<Node*> nodes;
  RunnableQueue scheduler;
  /// The currently executing runnable.
  Runnable *currentRunnable;
  PendingEvent pendingEvent;
  Timeout timeoutRunnable;
  SymbolInfo symbolInfo;

  uint8_t *rom;
  std::unique_ptr<DecodeCache> romDecodeCache;

  JIT jit;
  std::unique_ptr<Tracer> tracer;
  std::unique_ptr<Tracer> exitTracer;
#if AXE_ENABLE_SDL
  std::unique_ptr<SDLEventPoller> SDLPoller;
#endif

  void completeEvent(Thread &t, EventableResource &res, bool interrupt);

public:
  typedef std::vector<Node*>::iterator node_iterator;
  typedef std::vector<Node*>::const_iterator const_node_iterator;
  SystemState(std::unique_ptr<Tracer> tracer = std::unique_ptr<Tracer>());
  SystemState(const SystemState &) = delete;
  ~SystemState();

  void setExitTracer(std::unique_ptr<Tracer> exitTracer);

  void finalize();
  RunnableQueue &getScheduler() { return scheduler; }
  void addNode(std::unique_ptr<Node> n);

  SymbolInfo &getSymbolInfo() { return symbolInfo; }
  const SymbolInfo &getSymbolInfo() const { return symbolInfo; }

  Runnable *getExecutingRunnable() {
    return currentRunnable;
  }

  ticks_t getLatestThreadTime() const;

  void setTimeout(ticks_t time);
  void clearTimeout();

  StopReason run();

  /// Schedule a thread.
  void schedule(Thread &thread) {
    thread.waiting() = false;
    thread.pausedOn = 0;
    scheduler.push(thread, thread.time);
  }

  void deschedule(Thread &thread) {
    if(scheduler.contains(thread))
      scheduler.remove(thread);
  }

  bool schedulerContains(Thread &thread) {
    return scheduler.contains(thread);
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
              uint32_t romBase = 0xfff00000);
  const DecodeCache::State &getRomDecodeCache() const {
    return romDecodeCache->getState();
  }

  JIT &getJIT() { return jit; }
  Tracer *getTracer() { return tracer.get(); }
#if AXE_ENABLE_SDL
  SDLEventPoller *initSDL();
#endif

  const std::vector<Node*> &getNodes() const { return nodes; }
};

} // End axe namespace

#endif // _SystemState_h_
