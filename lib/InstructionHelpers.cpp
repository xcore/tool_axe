// Copyright (c) 2011-12, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "InstructionHelpers.h"
#include "Thread.h"
#include "Core.h"
#include "Exceptions.h"
#include "Trace.h"
#include "Synchroniser.h"
#include "Chanend.h"
#include "ClockBlock.h"
#include <iostream>
#include <cstdlib>

using namespace axe;
using namespace Register;

uint32_t axe::exception(Thread &t, uint32_t pc, int et, uint32_t ed)
{
  uint32_t sed = t.regs[ED];
  uint32_t spc = t.fromPc(pc);
  uint32_t ssr = t.sr.to_ulong();

  Tracer &tracer = t.getParent().getTracer();
  if (tracer.getTracingEnabled()) {
    tracer.exception(t, et, ed, sed, ssr, spc);
  }

  t.regs[SSR] = sed;
  t.regs[SPC] = spc;
  t.regs[SED] = sed;
  t.ink() = true;
  t.eeble() = false;
  t.ieble() = false;
  t.regs[ET] = et;
  t.regs[ED] = ed;

  uint32_t newPc = t.regs[KEP];
  if (et == ET_KCALL)
    newPc += 64;
  // TODO handle exception in ROM.
  if ((newPc & 1) || !t.getParent().isValidRamAddress(newPc)) {
    std::cout << "Error: unable to handle exception (invalid kep)\n";
    std::abort();
  }
  return t.toPc(newPc);
}

Resource *axe::checkResource(Core &state, ResourceID id)
{
  Resource *res = state.getResourceByID(id);
  if (!res || !res->isInUse())
    return 0;
  return res;
}

Synchroniser *axe::checkSync(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (res->getType() != RES_TYPE_SYNC)
    return 0;
  return static_cast<Synchroniser *>(res);
}

Thread *axe::checkThread(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (res->getType() != RES_TYPE_THREAD)
    return 0;
  return static_cast<Thread *>(res);
}

Chanend *axe::checkChanend(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (res->getType() != RES_TYPE_CHANEND)
    return 0;
  return static_cast<Chanend *>(res);
}

Port *axe::checkPort(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (res->getType() != RES_TYPE_PORT)
    return 0;
  return static_cast<Port *>(res);
}

EventableResource *axe::checkEventableResource(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (!res->isEventable())
    return 0;
  return static_cast<EventableResource *>(res);
}

const uint32_t CLK_REF = 0x1;

bool axe::setClock(Thread &t, ResourceID resID, uint32_t val, ticks_t time)
{
  Core &state = t.getParent();
  Resource *res = checkResource(state, resID);
  if (!res) {
    return false;
  }
  switch (res->getType()) {
    default: return false;
    case RES_TYPE_CLKBLK:
    {
      ClockBlock *c = static_cast<ClockBlock*>(res);
      if (val == CLK_REF) {
        c->setSourceRefClock(t, time);
        return true;
      }
      Port *p = checkPort(state, val);
      if (!p || p->getPortWidth() != 1)
        return false;
      return c->setSource(t, p, time);
    }
    case RES_TYPE_PORT:
    {
      Resource *source = state.getResourceByID(val);
      if (!source)
        return false;
      if (source->getType() != RES_TYPE_CLKBLK)
        return false;
      static_cast<Port*>(res)->setClk(t, static_cast<ClockBlock*>(source),
                                      time);
      return true;
    }
      break;
  }
  return true;
}

bool axe::setReadyInstruction(Thread &t, ResourceID resID, uint32_t val,
                         ticks_t time)
{
  Core &state = t.getParent();
  Resource *res = checkResource(state, resID);
  Port *ready = checkPort(state, val);
  if (!res || !ready) {
    return false;
  }
  return res->setReady(t, ready, time);
}

/// Set proccessor state. Can't be called from JIT as setting the RAM base
/// invalidates the cache.
bool axe::setProcessorState(Thread &t, uint32_t reg, uint32_t value)
{
  if (reg == PS_RAM_BASE && t.isInRam()) {
    // TODO changing ram base while executing from RAM requires more thought.
    // If we try and fetch the next instruction as normal then we will get an
    // illegal pc exception immediately. Instead we should execute the next
    // instruction in the instruction buffer, which would normally be a branch
    // instruction. For now just bailout.
    std::abort();
  }
  return t.getParent().setProcessorState(reg, value);
}
