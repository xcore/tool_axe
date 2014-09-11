// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "axe-c/axe.h"
#include "XE.h"
#include "XEReader.h"
#include "SystemState.h"
#include "SystemStateWrapper.h"
#include "ProcessorNode.h"
#include "Core.h"
#include "Node.h"
#include "Thread.h"
#include "StopReason.h"
#include "Resource.h"

#include "Tracer.h"
#include "LoggingTracer.h"
#include <cassert>

using namespace axe;

AXESystemRef axeCreateInstance(const char *xeFileName, int tracingEnabled)
{
  XE xe(xeFileName);
  XEReader xeReader(xe);
  SystemStateWrapper *sysWrapper;
  if(tracingEnabled != 0)
  {
    std::auto_ptr<Tracer> tracer = std::auto_ptr<Tracer>(new LoggingTracer(false));
    sysWrapper = new SystemStateWrapper(xeReader.readConfig(tracer));
  }
  else
  {
    sysWrapper = new SystemStateWrapper(xeReader.readConfig());
  }  
  return wrap(sysWrapper);
}

void axeRemoveThreadFromRunQueue(AXEThreadRef thread)
{
  SystemState *sys = unwrap(thread)->getParent().getParent()->getParent();
  sys->deschedule(*unwrap(thread));
}

void axeAddThreadToRunQueue(AXEThreadRef thread)
{
  SystemState *sys = unwrap(thread)->getParent().getParent()->getParent();
  sys->schedule(*unwrap(thread));
}

int axeIsThreadInRunQueue(AXEThreadRef thread)
{
  SystemState *sys = unwrap(thread)->getParent().getParent()->getParent();
  return sys->schedulerContains(*unwrap(thread));
}

void axeDeleteInstance(AXESystemRef system)
{
  delete unwrap(system);
}

int axeGetNumNodes(AXESystemRef system) {
  SystemState *sys = unwrap(system)->getSystemState();
  return sys->getNodes().size();
}

AXENodeType axeGetNodeType(AXESystemRef system, int nodeID) {
  SystemState *sys = unwrap(system)->getSystemState();
  const std::vector<Node*> nodes = sys->getNodes();
  
  if(nodes.size() < nodeID)
    return AXE_NODE_TYPE_UNKNOWN;

  Node *n = nodes.at(nodeID);
  if(!n)
    return AXE_NODE_TYPE_UNKNOWN;

  return (AXENodeType)n->getType();
}

int axeGetNumTiles(AXESystemRef system, int nodeID) {
  SystemState *sys = unwrap(system)->getSystemState();
  const std::vector<Node*> nodes = sys->getNodes();
  
  if(nodes.size() < nodeID)
    return 0;

  Node *n = nodes.at(nodeID);
  if(!n || !n->isProcessorNode())
    return -1;
  
  // Cast the Node to a ProcessorNode subclass, now that we know it is one
  return static_cast<ProcessorNode*>(n)->getCores().size();
}

int axeGetThreadInUse(AXEThreadRef thread) {
  return unwrap(thread)->isInUse();
}

int axeGetThreadID(AXEThreadRef thread) {
  Thread * t = unwrap(thread);

  return t->getNum();
}

AXECoreRef axeGetThreadParent(AXEThreadRef thread)
{
  return wrap(&unwrap(thread)->getParent());
}

AXECoreRef axeLookupCore(AXESystemRef system, unsigned jtagIndex, unsigned core)
{
  SystemState *sys = unwrap(system)->getSystemState();
  for (Node *node : sys->getNodes()) {
    if (!node->isProcessorNode())
      continue;
    if (node->getJtagIndex() != jtagIndex)
      continue;
    const std::vector<Core*> &cores =
      static_cast<ProcessorNode*>(node)->getCores();
    if (core >= cores.size())
      return 0;
    return wrap(cores[core]);
  }
  return 0;
}

int axeWriteMemory(AXECoreRef core, unsigned address, const void *src,
                   unsigned length)
{
  return unwrap(core)->writeMemory(address, src, length);
}

int axeReadMemory(AXECoreRef core, unsigned address, void *dst, unsigned length)
{
  return unwrap(core)->readMemory(address, dst, length);
}

int axeSetBreakpoint(AXECoreRef core, unsigned address)
{
  return unwrap(core)->setBreakpoint(address);
}

void axeUnsetBreakpoint(AXECoreRef core, unsigned address)
{
  return unwrap(core)->unsetBreakpoint(address);
}

void axeUnsetAllBreakpoints(AXESystemRef system)
{
  SystemState *sys = unwrap(system)->getSystemState();
  for(Node *n : sys->getNodes())
    if(n->isProcessorNode())
      for(Core *c : static_cast<ProcessorNode*>(n)->getCores())
        c->clearBreakpoints();
}

int axeSetWatchpoint(AXECoreRef core, AXEWatchpointType type, unsigned int startAddress, unsigned int endAddress)
{
  // We need to switch to "slow" tracing mode
  Core *c = unwrap(core);
  return c->setWatchpoint((WatchpointType)type, startAddress, endAddress);
}

void axeUnsetWatchpoint(AXECoreRef core, AXEWatchpointType type, unsigned int startAddress, unsigned int endAddress)
{
  // If there are no more watch points in the set, we should go back to "fast" non-tracing mode
  Core *c = unwrap(core);
  c->unsetWatchpoint((WatchpointType)type, startAddress, endAddress);
}

void axeUnsetAllWatchpoints(AXECoreRef core)
{
  Core *c = unwrap(core);
  c->clearWatchpoints();
}


void axeStepThreadOnce(AXEThreadRef thread)
{
  SystemState *sys = unwrap(thread)->getParent().getParent()->getParent();
  Thread *t = unwrap(thread);
  // To prevent this thread yeilding to others while single stepping, we need to
  //  set it's last run time to the oldest in the Runable Queue.
  t->setTime(sys->getEarliestThreadTime());
  t->singleStep();
}

AXEThreadRef axeLookupThread(AXECoreRef core, unsigned threadNum)
{
  return wrap(&unwrap(core)->getThread(threadNum));
}

int axeThreadIsInUse(AXEThreadRef thread)
{
  return unwrap(thread)->isInUse();
}

static bool convertRegNum(AXERegister axeReg, Register::Reg &reg) {
  using namespace Register;
  switch (axeReg) {
  case AXE_REG_R0:
  case AXE_REG_R1:
  case AXE_REG_R2:
  case AXE_REG_R3:
  case AXE_REG_R4:
  case AXE_REG_R5:
  case AXE_REG_R6:
  case AXE_REG_R7:
  case AXE_REG_R8:
  case AXE_REG_R9:
  case AXE_REG_R10:
  case AXE_REG_R11:
  case AXE_REG_CP:
  case AXE_REG_DP:
  case AXE_REG_SP:
  case AXE_REG_LR:
    reg = static_cast<Register::Reg>(R0 + (axeReg - AXE_REG_R0));
    return true;
  case AXE_REG_PC:
    return false;
  case AXE_REG_SR:
    return false;
  case AXE_REG_SPC:
    reg = SPC; return true;
  case AXE_REG_SSR:
    reg = SSR; return true;
  case AXE_REG_ET:
    reg = ET; return true;
  case AXE_REG_ED:
    reg = ED; return true;
  case AXE_REG_SED:
    reg = SED; return true;
  case AXE_REG_KEP:
    reg = KEP; return true;
  case AXE_REG_KSP:
    reg = KSP; return true;
  }
  return false;
}

int axeWriteReg(AXEThreadRef thread, AXERegister axeReg, unsigned value)
{
  switch (axeReg) {
  default:
    {
      Register::Reg regNum;
      if (!convertRegNum(axeReg, regNum))
        return 0;
      unwrap(thread)->reg(regNum) = value;
      return 1;
    }
  case AXE_REG_PC:
    unwrap(thread)->setPcFromAddress(value);
    return 1;
  case AXE_REG_SR:
    return 0;
  }
}

unsigned axeReadReg(AXEThreadRef thread, AXERegister axeReg)
{
  switch (axeReg) {
  default:
    {
      Register::Reg regNum;
      if (!convertRegNum(axeReg, regNum))
        return 0;
      return unwrap(thread)->reg(regNum);
    }
  case AXE_REG_PC:
    return unwrap(thread)->getRealPc();
  case AXE_REG_SR:
    return unwrap(thread)->sr.to_ulong();
  }
}

static AXEStopReason convertStopReasonType(StopReason::Type reason)
{
  switch (reason) {
  case StopReason::BREAKPOINT:
    return AXE_STOP_BREAKPOINT;
  case StopReason::TIMEOUT:
    return AXE_STOP_TIMEOUT;
  case StopReason::EXIT:
    return AXE_STOP_EXIT;
  case StopReason::NO_RUNNABLE_THREADS:
    return AXE_STOP_NO_RUNNABLE_THREADS;
  }
}

AXEStopReason axeRun(AXESystemRef system, unsigned maxCycles)
{
  StopReason::Type reason = unwrap(system)->run(maxCycles);
  return convertStopReasonType(reason);
}

AXEThreadRef axeGetThreadForLastBreakpoint(AXESystemRef system)
{
  return wrap(unwrap(system)->getThreadForLastBreakpoint());
}
