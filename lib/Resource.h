// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Resource_h_
#define _Resource_h_

#include <stdint.h>
#include <cassert>
#include "Runnable.h"
#include "Config.h"

namespace axe {

enum ResourceType {
  RES_TYPE_PORT = 0,
  RES_TYPE_TIMER = 1,
  RES_TYPE_CHANEND = 2,
  RES_TYPE_SYNC = 3,
  RES_TYPE_THREAD = 4,
  RES_TYPE_LOCK = 5,
  RES_TYPE_CLKBLK = 6,
  RES_TYPE_PS = 11,
  RES_TYPE_CONFIG = 12
};

enum ResConfigType {
  RES_CONFIG_PSCTRL = 0xc2,
  RES_CONFIG_SSCTRL = 0xc3,
};

const int LAST_STD_RES_TYPE = RES_TYPE_CLKBLK;

class ResourceID {
private:
  static const unsigned RES_TYPE_SHIFT = 0;
  static const unsigned RES_TYPE_SIZE = 8;
  static const unsigned RES_TYPE_MASK = (1 << RES_TYPE_SIZE) - 1;

  static const unsigned RES_NUM_SHIFT = 8;
  static const unsigned RES_NUM_SIZE = 8;
  static const unsigned RES_NUM_MASK = (1 << RES_NUM_SIZE) - 1;

  static const unsigned RES_CHANEND_NODE_SHIFT = 16;
  static const unsigned RES_CHANEND_NODE_SIZE = 16;
  static const unsigned RES_CHANEND_NODE_MASK = (1 << RES_CHANEND_NODE_SIZE) - 1;

  static const unsigned RES_PORT_WIDTH_SHIFT = 16;
  static const unsigned RES_PORT_WIDTH_SIZE = 6;
  static const unsigned RES_PORT_WIDTH_MASK = (1 << RES_PORT_WIDTH_SHIFT) - 1;

public:
  uint32_t id;
  ResourceID(uint32_t val) : id(val) {}

  ResourceType type() const
  {
    return (ResourceType)((id >> RES_TYPE_SHIFT) & RES_TYPE_MASK);
  }

  bool isChanend() const { return type() == RES_TYPE_CHANEND; }
  bool isConfig() const { return type() == RES_TYPE_CONFIG; }
  bool isChanendOrConfig() const {
    return type() == RES_TYPE_CHANEND || type() == RES_TYPE_CONFIG;
  }

  unsigned num() const
  {
    return (id >> RES_NUM_SHIFT) & RES_NUM_MASK;
  }

  void setNum(unsigned value)
  {
    value &= RES_NUM_MASK;
    id &= ~(RES_NUM_MASK << RES_NUM_SHIFT);
    id |= value << RES_NUM_SHIFT;
  }
  
  void setNode(unsigned value)
  {
    assert(isChanendOrConfig());
    value &= RES_CHANEND_NODE_MASK;
    id &= ~(RES_CHANEND_NODE_MASK << RES_CHANEND_NODE_SHIFT);
    id |= value << RES_CHANEND_NODE_SHIFT;
  }

  unsigned node() const
  {
    assert(isChanendOrConfig());
    return (id >> RES_CHANEND_NODE_SHIFT) & RES_CHANEND_NODE_MASK;
  }

  void setWidth(unsigned value)
  {
    assert((type() == RES_TYPE_PORT) && "width is only valid for ports");
    value &= RES_PORT_WIDTH_MASK;
    id &= ~(RES_PORT_WIDTH_MASK << RES_PORT_WIDTH_SHIFT);
    id |= value << RES_PORT_WIDTH_SHIFT;
  }

  unsigned width() const
  {
    assert((type() == RES_TYPE_PORT) && "width is only valid for ports");
    return (id >> RES_PORT_WIDTH_SHIFT) & RES_PORT_WIDTH_MASK;
  }
  
  operator uint32_t() const { return id; }
  
  static ResourceID chanendID(unsigned num, uint16_t node)
  {
    return RES_TYPE_CHANEND | (num << RES_NUM_SHIFT) |
           (node << RES_CHANEND_NODE_SHIFT);
  }

  static ResourceID timerID(unsigned num)
  {
    return RES_TYPE_TIMER | (num << RES_NUM_SHIFT);
  }

  static ResourceID syncID(unsigned num)
  {
    return RES_TYPE_SYNC | (num << RES_NUM_SHIFT);
  }

  static ResourceID threadID(unsigned num)
  {
    return RES_TYPE_THREAD | (num << RES_NUM_SHIFT);
  }

  static ResourceID lockID(unsigned num)
  {
    return RES_TYPE_LOCK | (num << RES_NUM_SHIFT);
  }

  static ResourceID clockBlockID(unsigned num)
  {
    return RES_TYPE_CLKBLK | (num << RES_NUM_SHIFT);
  }
};

class Thread;
class Port;

/// Resource base class.
class Resource {
private:
  /// Has the resource been allocated / turned on?
  bool inUse;
  ResourceID ID;
public:
  static const char *getResourceName(ResourceType type);
  bool isInUse() const
  {
    return inUse;
  }

  ResourceID getID() const
  {
    return ID;
  }

  ResourceType getType() const
  {
    return ID.type();
  }

  unsigned getNum() const
  {
    return ID.num();
  }

  void setNum(unsigned value)
  {
    ID.setNum(value);
  }
  
  void setNode(unsigned value)
  {
    ID.setNode(value);
  }
  
  void setWidth(unsigned value)
  {
    ID.setWidth(value);
  }

  virtual bool isEventable() const
  {
    return false;
  }

  virtual bool setCInUse(Thread &thread, bool val, ticks_t time)
  {
    return false;
  }

  enum Condition {
    COND_FULL,
    COND_AFTER,
    COND_EQ,
    COND_NEQ
  };

  virtual bool setCondition(Thread &thread, Condition c, ticks_t time)
  {
    return false;
  }

  virtual bool setData(Thread &thread, uint32_t value, ticks_t time)
  {
    return false;
  }

  virtual bool setReady(Thread &thread, Port *p, ticks_t time)
  {
    return false;
  }

  virtual bool alloc(Thread &master)
  {
    return false;
  }

  virtual bool free()
  {
    return false;
  }

  virtual void cancel()
  {
  }

  enum ResOpResult {
    CONTINUE,
    DESCHEDULE,
    ILLEGAL
  };

  virtual ResOpResult in(Thread &thread, ticks_t time, uint32_t &value)
  {
    return ILLEGAL;
  }

  virtual ResOpResult out(Thread &thread, uint32_t value, ticks_t time)
  {
    return ILLEGAL;
  }
protected:
  Resource(ResourceType t) : inUse(0), ID(t) {}

  void setInUse(bool val)
  {
    inUse = val;
  }
};

class EventableResource : public Resource, public Runnable {
private:
  /// Event / interrupt vector.
  uint32_t vector;
  /// Environment vector.
  uint32_t EV;
  /// Are events enabled on the resource?
  bool eventsEnabled;
  /// If true the resource will generate interrupts, otherwise it will generate
  /// events.
  bool interruptMode;
  /// The thread which owns this resource. This is the last thread to perform
  /// an operation on the resource.
  Thread *owner;

  uint32_t getTruncatedEV(Thread &s) const;
  
  void clearOwner();
  void updateOwnerAux(Thread &t);

protected:
  EventableResource(ResourceType Type) :
    Resource(Type),
    owner(0),
    next(0),
    prev(0) {}

  void updateOwner(Thread &t)
  {
    if (owner == &t)
      return;
    updateOwnerAux(t);
  }

  bool eventsPermitted() const;

  /// Generate an event. Sets pending event if the owner is currently
  /// executing, otherwise the owner is scheduled.s
  void event(ticks_t time);

  void eventableSetInUse(Thread &t, bool val);
  void eventableSetInUseOn(Thread &t);
  void eventableSetInUseOff();

  virtual bool seeEventEnable(ticks_t time) = 0;

  void scheduleUpdate(ticks_t time);
public:
  Thread &getOwner()
  {
    return *owner;
  }
  virtual void completeEvent();

  void setVector(Thread &thread, uint32_t v);
  void setEV(Thread &thread, uint32_t ev);
  /// Set the interrupt mode on this resource.
  void setInterruptMode(Thread &thread, bool Enable);
  void eventDisable(Thread &thread);

  /// Enable events on this resource.
  void eventEnable(Thread &thread);
  
  /// Checks to see if the resource can event. This should only be called if
  /// the resource's owner is the current thread. If the resource can event then
  /// a pending event is set.
  bool checkEvent();

  bool seeOwnerEventEnable(ticks_t &time)
  {
    return seeEventEnable(time);
  }

  virtual bool isEventable() const
  {
    return true;
  }

  EventableResource *next;
  EventableResource *prev;
};
  
} // End axe namespace

#endif // _Resource_h_
