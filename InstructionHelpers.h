// Copyright (c) 2011-12, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _InstructionHelpers_h_
#define _InstructionHelpers_h_

#include <stdint.h>
#include "Config.h"

namespace axe {

class Thread;
class Core;
class ResourceID;
class Resource;
class Synchroniser;
class Thread;
class Chanend;
class Port;
class EventableResource;

uint32_t exception(Thread &t, uint32_t pc, int et, uint32_t ed);

Resource *checkResource(Core &state, ResourceID id);

Synchroniser *checkSync(Core &state, ResourceID id);

Thread *checkThread(Core &state, ResourceID id);

Chanend *checkChanend(Core &state, ResourceID id);

Port *checkPort(Core &state, ResourceID id);

EventableResource *checkEventableResource(Core &state, ResourceID id);

bool setClock(Thread &t, ResourceID resID, uint32_t val, ticks_t time);

bool setReadyInstruction(Thread &t, ResourceID resID, uint32_t val,
                         ticks_t time);

bool setProcessorState(Thread &t, uint32_t reg, uint32_t value);
  
} // End axe namespace

#endif // _InstructionHelpers_h_
