// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "SystemState.h"
#include "ProcessorNode.h"
#include "Core.h"
#include "Tracer.h"
#include "StopReason.h"
#include <iostream>
#if AXE_ENABLE_SDL
#include "SDLEventPoller.h"
#include <SDL.h>
#endif

using namespace axe;
using namespace Register;

SystemState::SystemState(std::unique_ptr<Tracer> t) :
  currentRunnable(0),
  rom(0),
  tracer(std::move(t))
{
  pendingEvent.set = false;
  if (tracer.get()) {
    tracer->attach(*this);
  }
}

SystemState::~SystemState()
{
  for (Node *node : nodes) {
    delete node;
  }
  delete[] rom;
}

void SystemState::setExitTracer(std::unique_ptr<Tracer> t)
{
  exitTracer = std::move(t);
  exitTracer->attach(*this);
}

void SystemState::finalize()
{
  for (Node *node : nodes) {
    node->finalize();
  }
}

void SystemState::addNode(std::unique_ptr<Node> n)
{
  n->setParent(this);
  nodes.push_back(n.get());
  n.release();
}

void SystemState::
completeEvent(Thread &t, EventableResource &res, bool interrupt)
{
  if (interrupt) {
    t.regs[SSR] = t.sr.to_ulong();
    t.regs[SPC] = t.fromPc(t.pc);
    t.regs[SED] = t.regs[ED];
    t.ieble() = false;
    t.inint() = true;
    t.ink() = true;
  } else {
    t.inenb() = 0;
  }
  t.eeble() = false;
  // EventableResource::completeEvent sets the ED and PC.
  res.completeEvent();
  if (tracer.get()) {
    if (interrupt) {
      tracer->interrupt(t, res, t.fromPc(t.pc), t.regs[SSR], t.regs[SPC],
                        t.regs[SED], t.regs[ED]);
    } else {
      tracer->event(t, res, t.fromPc(t.pc), t.regs[ED]);
    }
  }
}

ticks_t SystemState::getLatestThreadTime() const
{
  ticks_t time = 0;
  for (Node *node : nodes) {
    if (!node->isProcessorNode())
      continue;
    for (Core *core : static_cast<ProcessorNode*>(node)->getCores()) {
      for (Thread &thread : core->getThreads()) {
        time = std::max(time, thread.time);
      }
    }
  }
  return time;
}

void SystemState::setTimeout(ticks_t time)
{
  scheduler.push(timeoutRunnable, time);
}

void SystemState::clearTimeout()
{
  // TODO do this in a less hacky way
  scheduler.push(timeoutRunnable, 0);
  scheduler.remove(timeoutRunnable);
}

StopReason SystemState::run()
{
  try {
    while (!scheduler.empty()) {
      Runnable &runnable = scheduler.front();
      currentRunnable = &runnable;
      scheduler.pop();
      runnable.run(runnable.wakeUpTime);
    }
  } catch (ExitException &ee) {
    return StopReason::getExit(ee.getTime(), ee.getStatus());
  } catch (TimeoutException &te) {
    if (scheduler.empty()) {
      if (tracer.get())
        tracer->noRunnableThreads(*this);
      if (exitTracer.get())
        exitTracer->noRunnableThreads(*this);
      return StopReason::getNoRunnableThreads(te.getTime());
    }
    if (tracer.get())
      tracer->timeout(*this, te.getTime());
    if (exitTracer.get())
      exitTracer->timeout(*this, te.getTime());
    return StopReason::getTimeout(te.getTime());
  } catch (BreakpointException &be) {
    return StopReason::getBreakpoint(be.getTime(), be.getThread());
  } catch (WatchpointException &we) {
    return StopReason::getWatchpoint(we.getTime(), we.getThread());
  }
  if (tracer.get())
    tracer->noRunnableThreads(*this);
  if (exitTracer.get())
    exitTracer->noRunnableThreads(*this);
  return StopReason::getNoRunnableThreads(getLatestThreadTime());
}

void SystemState::
setRom(const uint8_t *data, uint32_t romSize, uint32_t romBase)
{
  rom = new uint8_t[romSize];
  std::memcpy(rom, data, romSize);
  romDecodeCache.reset(new DecodeCache(romSize, romBase, false,
                                       tracer.get() != nullptr));
  for (Node *node : nodes) {
    if (!node->isProcessorNode())
      continue;
    for (Core *core : static_cast<ProcessorNode*>(node)->getCores()) {
      core->setRom(rom, romBase, romSize);
    }
  }
}

#if AXE_ENABLE_SDL
SDLEventPoller *SystemState::initSDL()
{
  if (!SDLPoller.get()) {
    if (SDL_Init(0) < 0) {
      std::cerr << "Failed to initialize SDL\n";
      std::exit(1);
    }
    SDLPoller.reset(new SDLEventPoller(scheduler));
  }
  return SDLPoller.get();
}
#endif
