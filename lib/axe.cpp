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
#include <cassert>

using namespace axe;

AXESystemRef axeCreateInstance(const char *xeFileName)
{
  XE xe(xeFileName);
  XEReader xeReader(xe);
  SystemStateWrapper *sysWrapper =
    new SystemStateWrapper(xeReader.readConfig());
  return wrap(sysWrapper);
}

void axeDeleteInstance(AXESystemRef system)
{
  delete unwrap(system);
}

int axeGetNumNodes(AXESystemRef system) {
  SystemState *sys = unwrap(system)->getSystemState();
  return sys->getNodes().size();
}

int axeGetNodeType(AXESystemRef system, int nodeID) {
  SystemState *sys = unwrap(system)->getSystemState();
  const std::vector<Node*> nodes = sys->getNodes();

  Node *n = nodes.at(nodeID);
  if(!n)
    return -1;

  return n->getType();
}

int axeGetNumTiles(AXESystemRef system, int nodeID) {
  SystemState *sys = unwrap(system)->getSystemState();
  const std::vector<Node*> nodes = sys->getNodes();

  Node *n = nodes.at(nodeID);
  if(!n || !n->isProcessorNode())
    return -1;
  
  // Cast the Node to a ProcessorNode subclass, now that we know it is one
  return static_cast<ProcessorNode*>(n)->getCores().size();
}

bool axeGetThreadInUse(AXEThreadRef thread) {
  return unwrap(thread)->isInUse();
}

void axeDisableNode(AXESystemRef system, int nodeID) {
  SystemState *sys = unwrap(system)->getSystemState();
  const std::vector<Node*> nodes = sys->getNodes();

  Node *n = nodes.at(nodeID);
  if(!n)
    return;

  // n->setEnabled(false);
}

void axeEnableNode(AXESystemRef system, int nodeID) {
  SystemState *sys = unwrap(system)->getSystemState();
  const std::vector<Node*> nodes = sys->getNodes();

  Node *n = nodes.at(nodeID);
  if(!n)
    return;

  // n->setEnabled(true);
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

void axeScheduleThread(AXEThreadRef thread)
{
  unwrap(thread)->schedule();
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
