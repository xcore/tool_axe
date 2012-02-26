// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Thread_h_
#define _Thread_h_

#include <stdint.h>
#include <ostream>
#include <bitset>
#include "Runnable.h"
#include "RunnableQueue.h"
#include "Resource.h"

class Synchroniser;

class ExitException {
  unsigned status;
public:
  ExitException(unsigned s) : status(s) {}
  unsigned getStatus() const { return status; }
};

enum Register {
  R0,
  R1,
  R2,
  R3,
  R4,
  R5,
  R6,
  R7,
  R8,
  R9,
  R10,
  R11,
  CP,
  DP,
  SP,
  LR,
  ET,
  ED,
  KEP,
  KSP,
  SPC,
  SED,
  SSR,
  NUM_REGISTERS
};

extern const char *registerNames[];

inline const char *getRegisterName(unsigned RegNum) {
  if (RegNum < NUM_REGISTERS) {
    return registerNames[RegNum];
  }
  return "?";
}

inline std::ostream &operator<<(std::ostream &out, const Register &r)
{
  return out << getRegisterName(r);
}

class EventableResourceIterator :
  public std::iterator<std::forward_iterator_tag, int> {
public:
  EventableResourceIterator() : p(0) {}
  EventableResourceIterator(EventableResource *res) : p(res) {}
  bool operator==(const EventableResourceIterator &it) { return p == it.p; }
  bool operator!=(const EventableResourceIterator &it) { return p != it.p; }
  const EventableResourceIterator &operator++() {
    p = p->next;
    return *this;
  }
  EventableResource *&operator*() {
    return p;
  }
private:
  EventableResource *p;
};

class EventableResourceList {
public:
  EventableResourceList() : head(0) {}
  void add(EventableResource *res)
  {
    res->next = head;
    res->prev = 0;
    if (head)
      head->prev = res;
    head = res;
  }

  void remove(EventableResource *res)
  {
    if (res->prev) {
      res->prev->next = res->next;
    } else {
      head = res->next;
    }
    if (res->next) {
      res->next->prev = res->prev;
    }
  }
  typedef EventableResourceIterator iterator;
  iterator begin() { return EventableResourceIterator(head); }
  iterator end() { return EventableResourceIterator(); }
private:
  EventableResource *head;
};

class Core;
class RunnableQueue;

class Thread : public Runnable, public Resource {
  bool ssync;
  Synchroniser *sync;
  /// Resources owned by the thread with events enabled.
  EventableResourceList eventEnabledResources;
  /// Resources owned by the thread with interrupts enabled.
  EventableResourceList interruptEnabledResources;
  /// Parent core.
  Core *parent;
  /// The scheduler.
  RunnableQueue *scheduler;
public:
  enum SRBit {
    EEBLE = 0,
    IEBLE = 1,
    INENB = 2,
    ININT = 3,
    INK = 4,
    // TODO When is this set?
    SINK = 5,
    WAITING = 6,
    FAST = 7
  };
  typedef std::bitset<8> sr_t;
  uint32_t regs[NUM_REGISTERS];
  /// The program counter. Note that the pc will not be valid if the thread is
  /// executing since it is cached in the dispatch loop.
  uint32_t pc;
  /// The time for the thread. This approximates the XCore's 400 MHz processor
  /// clock.
  ticks_t time;
  sr_t sr;
  /// When executing some pseduo instructions placed at the end this holds the
  /// real pc.
  uint32_t pendingPc;
  /// The resource on which the thread is paused on.
  Resource *pausedOn;

  Thread() : Resource(RES_TYPE_THREAD), parent(0), scheduler(0) {
    time = 0;
    pc = 0;
    regs[KEP] = 0;
    regs[KSP] = 0;
    regs[SPC] = 0;
    regs[SED] = 0;
    regs[ET] = 0;
    regs[ED] = 0;
    eeble() = false;
    ieble() = false;
    setInUse(false);
  }
  
  bool hasTimeSliceExpired() const {
    if (scheduler->empty())
      return false;
    return time > scheduler->front().wakeUpTime;
  }

  bool alloc(Thread &CurrentThread)
  {
    alloc(CurrentThread.time);
    return true;
  }

  bool free()
  {
    setInUse(false);
    return true;
  }

  void setParent(Core &p) { parent = &p; }

  Core &getParent() { return *parent; }
  const Core &getParent() const { return *parent; }

  void finalize();

  void addEventEnabledResource(EventableResource *res)
  {
    eventEnabledResources.add(res);
  }
  
  void removeEventEnabledResource(EventableResource *res)
  {
    eventEnabledResources.remove(res);
  }
  
  void addInterruptEnabledResource(EventableResource *res)
  {
    interruptEnabledResources.add(res);
  }
  
  void removeInterruptEnabledResource(EventableResource *res)
  {
    interruptEnabledResources.remove(res);
  }

  void alloc(ticks_t t)
  {
    assert(!isInUse() && "Trying to allocate in use thread");
    setInUse(true);
    sync = 0;
    ssync = true;
    time = t;
    pausedOn = 0;
  }

  void setSync(Synchroniser &s)
  {
    assert(!sync && "Synchroniser set twice");
    sync = &s;
  }
  
  bool inSSync() const
  {
    return ssync;
  }

  void setSSync(bool value)
  {
    ssync = value;
  }
  
  Synchroniser *getSync()
  {
    return sync;
  }

  uint32_t &reg(unsigned RegNum)
  {
    return regs[RegNum];
  }
  
  sr_t::reference ieble() {
    return sr[IEBLE];
  }
  
  bool ieble() const {
    return sr[IEBLE];
  }

  sr_t::reference eeble() {
    return sr[EEBLE];
  }

  bool eeble() const {
    return sr[EEBLE];
  }

  sr_t::reference ink() {
    return sr[INK];
  }

  sr_t::reference inint() {
    return sr[ININT];
  }

  sr_t::reference inenb() {
    return sr[INENB];
  }

  sr_t::reference waiting() {
    return sr[WAITING];
  }
  
  bool waiting() const {
    return sr[WAITING];
  }

public:
  void dump() const;

  void schedule();
  void takeEvent();
  bool hasPendingEvent() const;
  
  /// Enable for events on the current thread.
  /// \return true if there is a pending event, false otherwise.
  bool enableEvents()
  {
    sr_t newSR = sr;
    newSR[EEBLE] = true;
    return setSR(newSR);
  }

  /// Set the SR register.
  /// \return true if there is a pending event, false otherwise.
  bool setSR(sr_t value)
  {
    sr_t enabled = value & (sr ^ value);
    sr = value;
    if (!enabled[EEBLE] && !enabled[IEBLE])
      return false;
    return setSRSlowPath(enabled);
  }

public:
  void clre()
  {
    eeble() = false;
    inenb() = false;
    EventableResourceList &EER = eventEnabledResources;
    for (EventableResourceList::iterator it = EER.begin(),
         end = EER.end(); it != end; ++it) {
      (*it)->eventDisable(*this);
    }
  }
  bool isExecuting() const;
  void run(ticks_t time);
  bool setC(ticks_t time, ResourceID resID, uint32_t val);
private:
  template <bool tracing> void runAux(ticks_t time);
  bool setSRSlowPath(sr_t old);
};

struct PendingEvent {
  EventableResource *res;
  bool set;
  bool interrupt;
  ticks_t time;
};

void initInstructionCache(Core &c);

#endif // _Thread_h_
