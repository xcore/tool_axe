// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "SystemState.h"
#include "Node.h"
#include "Core.h"
#include "Trace.h"
#include "StopReason.h"

using namespace axe;
using namespace Register;

SystemState::SystemState(bool tracing) :
  currentRunnable(0),
  rom(0),
  tracer(new Tracer(tracing)) {
  pendingEvent.set = false;
}

SystemState::~SystemState()
{
  for (Node *node : nodes) {
    delete node;
  }
  delete[] rom;
  delete tracer;
}

void SystemState::finalize()
{
  for (Node *node : nodes) {
    node->finalize();
  }
}

void SystemState::addNode(std::auto_ptr<Node> n)
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
  if (tracer->getTracingEnabled()) {
    if (interrupt) {
      tracer->interrupt(t, res, t.fromPc(t.pc), t.regs[SSR], t.regs[SPC],
                        t.regs[SED], t.regs[ED]);
    } else {
      tracer->event(t, res, t.fromPc(t.pc), t.regs[ED]);
    }
  }
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
    return StopReason::getExit(ee.getStatus());
  } catch (TimeoutException &te) {
    tracer->timeout(*this, te.getTime());
    return StopReason::getTimeout();
  } catch (BreakpointException &be) {
    return StopReason::getBreakpoint(be.getThread());
  }
  tracer->noRunnableThreads(*this);
  return StopReason::getNoRunnableThreads();
}

void SystemState::
setRom(const uint8_t *data, uint32_t romSize, uint32_t romBase)
{
  rom = new uint8_t[romSize];
  std::memcpy(rom, data, romSize);
  romDecodeCache.reset(new DecodeCache(romSize, romBase, false,
                                       tracer->getTracingEnabled()));
  for (Node *node : nodes) {
    for (auto it = node->core_begin(), e = node->core_end(); it != e; ++it) {
      Core *core = *it;
      core->setRom(rom, romBase, romSize);
    }
  }
}
