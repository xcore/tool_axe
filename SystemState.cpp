// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "SystemState.h"
#include "Node.h"
#include "Core.h"
#include "Trace.h"

using namespace Register;

SystemState::~SystemState()
{
  for (node_iterator it = nodes.begin(), e = nodes.end(); it != e; ++it) {
    delete *it;
  }
}

void SystemState::finalize()
{
  for (node_iterator it = nodes.begin(), e = nodes.end(); it != e; ++it) {
    (*it)->finalize();
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
    t.regs[SPC] = t.getParent().targetPc(t.pc);
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
  if (Tracer::get().getTracingEnabled()) {
    if (interrupt) {
      Tracer::get().interrupt(t, res, t.getParent().targetPc(t.pc),
                                      t.regs[SSR], t.regs[SPC], t.regs[SED],
                                      t.regs[ED]);
    } else {
      Tracer::get().event(t, res, t.getParent().targetPc(t.pc), t.regs[ED]);
    }
  }
}

ChanEndpoint *SystemState::getChanendDest(ResourceID ID)
{
  unsigned coreID = ID.node();
  // TODO build lookup map.

  for (node_iterator outerIt = node_begin(), outerE = node_end();
       outerIt != outerE; ++outerIt) {
    Node &node = **outerIt;
    for (Node::core_iterator innerIt = node.core_begin(),
         innerE = node.core_end(); innerIt != innerE; ++innerIt) {
      Core &core = **innerIt;
      if (core.getCoreID() == coreID) {
        ChanEndpoint *result;
        bool isLocal = core.getLocalChanendDest(ID, result);
        assert(isLocal);
        (void)isLocal;
        return result;
      }
    }
  }
  return 0;
}

int SystemState::run()
{
  try {
    while (!scheduler.empty()) {
      Runnable &runnable = scheduler.front();
      currentRunnable = &runnable;
      scheduler.pop();
      runnable.run(runnable.wakeUpTime);
    }
  } catch (ExitException &ee) {
    return ee.getStatus();
  }
  Tracer::get().noRunnableThreads(*this);
  return 1;
}
