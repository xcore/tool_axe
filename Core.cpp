// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Core.h"
#include "SystemState.h"
#include "Node.h"
#include <iostream>
#include <iomanip>

bool Core::allocatable[LAST_STD_RES_TYPE + 1] = {
  false, // RES_TYPE_PORT
  true, // RES_TYPE_TIMER
  true, // RES_TYPE_CHANEND
  true, // RES_TYPE_SYNC
  true, // RES_TYPE_THREAD
  true, // RES_TYPE_LOCK
  false, // RES_TYPE_CLKBLK
};

void Core::dumpPaused() const
{
  for (unsigned i = 0; i < NUM_THREADS; i++) {
    Thread *t = static_cast<Thread*>(resource[RES_TYPE_THREAD][i]);
    if (!t->isInUse())
      continue;
    std::cout << "Thread " << std::dec << i;
    ThreadState &ts = t->getState();
    if (Resource *res = ts.pausedOn) {
      std::cout << " paused on ";
      std::cout << Resource::getResourceName(res->getType());
      std::cout << " 0x" << std::hex << res->getID();
    } else if (ts.eeble()) {
      std::cout << " waiting for events";
      if (ts.ieble())
        std::cout << " or interrupts";
    } else if (ts.ieble()) {
      std::cout << " waiting for interrupts";
    } else {
      std::cout << " paused on unknown";
    }
    std::cout << "\n";
  }
}

const Port *Core::getPortByID(ResourceID ID) const
{
  assert(ID.type() == RES_TYPE_PORT);
  unsigned width = ID.width();
  if (width > 32)
    return 0;
  unsigned num = ID.num();
  if (num > portNum[width])
    return 0;
  return &port[width][num];
}

const Resource *Core::getResourceByID(ResourceID ID) const
{
  ResourceType type = ID.type();
  if (type > LAST_STD_RES_TYPE) {
    return 0;
  }
  if (type == RES_TYPE_PORT) {
    return getPortByID(ID);
  }
  unsigned num = ID.num();
  if (num >= resourceNum[type]) {
    return 0;
  }
  return resource[type][num];
}

Resource *Core::getResourceByID(ResourceID ID)
{
  return const_cast<Resource *>(
    static_cast<const Core *>(this)->getResourceByID(ID)
  );
}

void Core::
initCache(OPCODE_TYPE decode, OPCODE_TYPE illegalPC,
          OPCODE_TYPE illegalPCThread, OPCODE_TYPE noThreads)
{
  const uint32_t ramSizeShorts = ram_size >> 1;
  // Initialise instruction cache.
  for (unsigned i = 0; i < ramSizeShorts; i++) {
    opcode[i] = decode;
  }
  opcode[ramSizeShorts] = illegalPC;
  opcode[getIllegalPCThreadAddr()] = illegalPCThread;
  opcode[getNoThreadsAddr()] = noThreads;
}

bool Core::getLocalChanendDest(ResourceID ID, ChanEndpoint *&result)
{
  assert(ID.isChanendOrConfig());
  if (ID.isConfig()) {
    switch (ID.num()) {
      case RES_CONFIG_SSCTRL:
        if (parent->hasMatchingNodeID(ID)) {
          result = parent->getSSwitch();
          return true;
        }
        break;
      case RES_CONFIG_PSCTRL:
        // TODO.
        assert(0);
        result = 0;
        return true;
    }
  } else {
    if (ID.node() == getCoreID()) {
      Resource *res = getResourceByID(ID);
      if (res) {
        assert(res->getType() == RES_TYPE_CHANEND);
        result = static_cast<Chanend*>(res);
      } else {
        result = 0;
      }
      return true;
    }
  }
  return false;
}

ChanEndpoint *Core::getChanendDest(ResourceID ID)
{
  if (!ID.isChanendOrConfig())
    return 0;
  ChanEndpoint *result;
  // Try to lookup locally first.
  if (getLocalChanendDest(ID, result))
    return result;
  return parent->getParent()->getChanendDest(ID);
}

void Core::updateIDs()
{
  unsigned coreID = getCoreID();
  for (unsigned i = 0; i < NUM_CHANENDS; i++) {
    resource[RES_TYPE_CHANEND][i]->setNode(coreID);
  }    
}

uint32_t Core::getCoreID() const
{
  return getParent()->getCoreID(coreNumber);
}
