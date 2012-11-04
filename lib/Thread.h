// Copyright (c) 2011-12, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Thread_h_
#define _Thread_h_

#include <stdint.h>
#include <bitset>
#include "Runnable.h"
#include "RunnableQueue.h"
#include "Resource.h"
#include "Register.h"
#include "Instruction.h"
#include "DecodeCache.h"

namespace axe {

class Synchroniser;

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
struct Operands;

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
  /// Cached decode information.
  DecodeCache::State decodeCache;
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
  uint32_t regs[Register::NUM_REGISTERS];
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

  Thread();

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

  void setParent(Core &p);

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

  bool isInRam() const;

  bool tryUpdateDecodeCache(uint32_t address);

  void setPcFromAddress(uint32_t address);
  
  uint32_t fromPc(uint32_t pc) const {
    return decodeCache.fromPc(pc);
  }

  uint32_t toPc(uint32_t pc) const {
    return decodeCache.toPc(pc);
  }

  void setOpcode(uint32_t pc, OPCODE_TYPE opc, unsigned size) {
    decodeCache.setOpcode(pc, opc, size);
  }

  void setOpcode(uint32_t pc, OPCODE_TYPE opc, Operands &ops, unsigned size) {
    decodeCache.setOpcode(pc, opc, ops, size);
  }

  void runJIT(uint32_t pc);

  bool updateExecutionFrequencyFromStub(uint32_t shiftedAddress) {
    const DecodeCache::executionFrequency_t threshold = 128;
    DecodeCache::executionFrequency_t *executionFrequency =
    decodeCache.executionFrequency;
    if (++executionFrequency[shiftedAddress] > threshold) {
      return true;
    }
    return false;
  }

  void updateExecutionFrequency(uint32_t shiftedAddress) {
    if (updateExecutionFrequencyFromStub(shiftedAddress))
      runJIT(shiftedAddress);
  }

  void seeRamDecodeCacheChange();

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

  void clre()
  {
    eeble() = false;
    inenb() = false;
    EventableResourceList &EER = eventEnabledResources;
    for (EventableResource *res : EER) {
      res->eventDisable(*this);
    }
  }
  bool isExecuting() const;
  void run(ticks_t time);
  bool setC(ticks_t time, ResourceID resID, uint32_t val);

  const Operands &getOperands(uint32_t pc) const {
    return decodeCache.operands[pc];
  }
private:
  bool setSRSlowPath(sr_t old);
};

struct PendingEvent {
  EventableResource *res;
  bool set;
  bool interrupt;
  ticks_t time;
};

OPCODE_TYPE getInstruction_DECODE(bool tracing);
OPCODE_TYPE getInstruction_ILLEGAL_PC(bool tracing);
OPCODE_TYPE getInstruction_ILLEGAL_PC_THREAD(bool tracing);
OPCODE_TYPE getInstruction_RUN_JIT(bool tracing);
OPCODE_TYPE getInstruction_INTERPRET_ONE(bool tracing);
  
} // End axe namespace

#endif // _Thread_h_
