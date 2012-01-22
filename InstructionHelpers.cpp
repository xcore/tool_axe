// Copyright (c) 2011-12, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "InstructionHelpers.h"
#include "Thread.h"
#include "Core.h"
#include "Exceptions.h"

uint32_t exception(Thread &t, uint32_t pc, int et, uint32_t ed)
{
  uint32_t sed = t.regs[ED];
  uint32_t spc = t.getParent().targetPc(pc);
  uint32_t ssr = t.sr.to_ulong();

  if (Tracer::get().getTracingEnabled()) {
    Tracer::get().exception(t, et, ed, sed, ssr, spc);
  }

  t.regs[SSR] = sed;
  t.regs[SPC] = spc;
  t.regs[SED] = sed;
  t.ink() = true;
  t.eeble() = false;
  t.ieble() = false;
  t.regs[ET] = et;
  t.regs[ED] = ed;

  uint32_t newPc = t.getParent().physicalAddress(t.regs[KEP]);
  if (et == ET_KCALL)
    newPc += 64;
  if ((newPc & 1) || !t.getParent().isValidAddress(newPc)) {
    std::cout << "Error: unable to handle exception (invalid kep)\n";
    std::abort();
  }
  return newPc >> 1;
}

Resource *checkResource(Core &state, ResourceID id)
{
  Resource *res = state.getResourceByID(id);
  if (!res || !res->isInUse())
    return 0;
  return res;
}

Synchroniser *checkSync(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (res->getType() != RES_TYPE_SYNC)
    return 0;
  return static_cast<Synchroniser *>(res);
}

Thread *checkThread(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (res->getType() != RES_TYPE_THREAD)
    return 0;
  return static_cast<Thread *>(res);
}

Chanend *checkChanend(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (res->getType() != RES_TYPE_CHANEND)
    return 0;
  return static_cast<Chanend *>(res);
}

Port *checkPort(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (res->getType() != RES_TYPE_PORT)
    return 0;
  return static_cast<Port *>(res);
}

EventableResource *checkEventableResource(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (!res->isEventable())
    return 0;
  return static_cast<EventableResource *>(res);
}
