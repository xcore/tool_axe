// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Resource.h"
#include "Core.h"
#include "Node.h"
#include "SystemState.h"

using namespace axe;
using namespace Register;

const char *Resource::getResourceName(ResourceType type)
{
  switch (type) {
  case RES_TYPE_PORT: return "port";
  case RES_TYPE_TIMER: return "timer";
  case RES_TYPE_CHANEND: return "channel end";
  case RES_TYPE_SYNC: return "synchroniser";
  case RES_TYPE_THREAD: return "thread";
  case RES_TYPE_LOCK: return "lock";
  case RES_TYPE_CLKBLK: return "clock block";
  case RES_TYPE_PS: return "processor state";
  case RES_TYPE_CONFIG: return "config";
  }
}

void EventableResource::updateOwnerAux(Thread &t)
{
  if (eventsEnabled) {
    if (interruptMode) {
      owner->removeInterruptEnabledResource(this);
      t.addInterruptEnabledResource(this);
    } else {
      owner->removeEventEnabledResource(this);
      t.addEventEnabledResource(this);
    }
  }
  owner = &t;
}

void EventableResource::clearOwner()
{
  if (eventsEnabled) {
    if (interruptMode) {
      owner->removeInterruptEnabledResource(this);
    } else {
      owner->removeEventEnabledResource(this);
    }
  }
  owner = 0;
}

void EventableResource::setVector(Thread &thread, uint32_t v)
{
  updateOwner(thread);
  vector = v;
}

void EventableResource::setEV(Thread &thread, uint32_t ev)
{
  updateOwner(thread);
  EV = ev;
}

void EventableResource::setInterruptMode(Thread &thread, bool Enable)
{
  updateOwner(thread);
  if (Enable == interruptMode)
    return;
  if (eventsEnabled) {
    if (interruptMode) {
      owner->removeInterruptEnabledResource(this);
      owner->addEventEnabledResource(this);
    } else {
      owner->removeEventEnabledResource(this);
      owner->addInterruptEnabledResource(this);
    }
  }
  interruptMode = Enable;
  if (eventsPermitted() && seeEventEnable(thread.time)) {
    event(thread.time);
  }
}

bool EventableResource::eventsPermitted() const
{
  if (!owner)
    return false;
  return eventsEnabled && (interruptMode ? owner->ieble() : owner->eeble());
}

void EventableResource::event(ticks_t time)
{
  assert(eventsPermitted());
  SystemState &sys = *owner->getParent().getParent()->getParent();
  if (owner->isExecuting()) {
    sys.setPendingEvent(*this, time, interruptMode);
    return;
  }
  owner->time = time;
  sys.takeEvent(*owner, *this, interruptMode);
}

void EventableResource::completeEvent()
{
  owner->reg(ED) = getTruncatedEV(*owner);
  owner->pc = vector;
}

void EventableResource::eventableSetInUseOn(Thread &t)
{
  if (isInUse())
    clearOwner();
  vector = 0;
  EV = getID();
  eventsEnabled = false;
  interruptMode = false;
  owner = &t;
  Resource::setInUse(true);
}

void EventableResource::eventableSetInUseOff()
{
  if (!isInUse())
    return;
  clearOwner();
  Resource::setInUse(false);
}

void EventableResource::eventableSetInUse(Thread &t, bool val)
{
  if (val)
    eventableSetInUseOn(t);
  else
    eventableSetInUseOff();
}

uint32_t EventableResource::getTruncatedEV(Thread &thread) const
{
  if (EV == getID())
    return EV;
  return (EV & 0xffff) | thread.getParent().getRamBase();
}

void EventableResource::eventDisable(Thread &thread)
{
  if (eventsEnabled) {
    if (interruptMode) {
      owner->removeInterruptEnabledResource(this);
    } else {
      owner->removeEventEnabledResource(this);
    }
  }
  eventsEnabled = false;
  updateOwner(thread);
}

void EventableResource::eventEnable(Thread &thread)
{
  updateOwner(thread);
  if (!eventsEnabled) {
    if (interruptMode) {
      owner->addInterruptEnabledResource(this);
    } else {
      owner->addEventEnabledResource(this);
    }
  }
  eventsEnabled = true;
  if (eventsPermitted() && seeEventEnable(owner->time)) {
    event(owner->time);
  }
}

bool EventableResource::checkEvent()
{
  if (eventsPermitted() && seeEventEnable(owner->time)) {
    event(owner->time);
    return true;
  }
  return false;
}

void EventableResource::scheduleUpdate(ticks_t time)
{
  getOwner().getParent().getParent()->getParent()->scheduleOther(*this, time);
}

