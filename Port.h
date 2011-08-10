// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Port_h_
#define _Port_h_

#include "Resource.h"
#include "ClockBlock.h"
#include "BitManip.h"
#include <stdint.h>
#include <set>

class ThreadState;
class ClockBlock;
struct Signal;

class Port : public EventableResource {
public:
  enum ReadyMode {
    NOREADY,
    STROBED,
    HANDSHAKE
  };
  enum MasterSlave {
    MASTER,
    SLAVE
  };
  enum PortType {
    DATAPORT,
    READYPORT,
    CLOCKPORT
  };
private:
  uint32_t data;
  Condition condition;
  ClockBlock *clock;
  std::set<ClockBlock*> sourceOf;
  std::set<ClockBlock*> readyInOf;
  Port *readyOutOf;
  uint16_t portCounter;
  // Current value on the pins.
  uint32_t shiftRegister;
  /// Outputs pins connected to loopback port. 0 if no loopback port.
  Port *loopback;
  /// Ready out ports.
  std::set<Port*> readyOutPorts;
  /// Thread paused on an output instruction.
  ThreadState *pausedOut;
  /// Thread paused on an input instruction.
  ThreadState *pausedIn;
  /// Thread paused on a sync instruction.
  ThreadState *pausedSync;
  /// Number of port width entries in the transfer register.
  unsigned shiftRegEntries;
  /// Number of port width entries for the next input / output.
  unsigned nextShiftRegEntries;
  /// Number of valid port width entries in the transfer register.
  unsigned validShiftRegEntries;
  /// Shift register.
  uint32_t shiftReg;
  /// Transfer register.
  uint32_t transferReg;
  /// Port time register.
  uint16_t timeReg;
  /// Value of ready out on the last falling edge.
  bool readyOut;
  /// Transfer width
  unsigned transferWidth;
  /// Has the port time register been set?
  bool timeRegValid;
  /// Does the transfer register hold a valid value?
  bool transferRegValid;
  /// Timestamp register returned by getts.
  ticks_t timestampReg;
  /// Hold the transfer value until input / clrpt / setpt
  bool holdTransferReg;
  /// Time port has been updated to.
  ticks_t time;
  /// Next edge. The iterator is only valid if:
  /// The port is in use,
  /// The port is a data port,
  /// The clock is a fixed frequency clock.
  EdgeIterator nextEdge;
  /// If the port being used for inputs or outputs.
  bool outputPort;
  /// Is the port buffered?
  bool buffered;
  ReadyMode readyMode;
  MasterSlave masterSlave;
  PortType portType;
  Signal pinsInputValue;

  /// Return the value currently being output to the ports pins.
  Signal getPinsOutputValue() const;
  uint32_t getPinsOutputValue(ticks_t time) const {
    return getPinsOutputValue().getValue(time);
  }
  unsigned getReadyOutValue() const {
    return readyOut;
  }
  void handlePinsChange(Signal value, ticks_t time);
  /// Called whenever the value being output to the pins changes.
  void handlePinsChange(uint32_t value, ticks_t time);
  /// Called whenever the readyOut value changes.
  void handleReadyOutChange(bool value, ticks_t time);
  /// Update the pin buffer with the change.
  void seePinsChange(const Signal &value, ticks_t time);
  /// Update the port to the specified time. The port must be clocked off a
  /// fixed frequency clock.
  void updateAux(ticks_t time);

  /// Return whether the condition is met for the specified value.
  bool valueMeetsCondition(uint32_t value) const;
  bool conditionMet() const {
    if (!transferRegValid)
      return false;
    if (condition == COND_FULL)
      return true;
    return !isBuffered() && valueMeetsCondition(transferReg);
  }
  bool timeMet() const {
    if (!transferRegValid)
      return false;
    return (!timeRegValid || timestampReg == timeReg);
  }
  bool timeAndConditionMet() const {
    return timeMet() && conditionMet();
  }
  void setTransferReg(uint32_t value) {
    transferReg = value;
    transferRegValid = true;
  }
  void updateDependentClocks(const Signal &value, ticks_t time);
  unsigned fallingEdgesUntilTimeMet() const;
  void scheduleUpdateIfNeededOutputPort();
  void scheduleUpdateIfNeededInputPort();
  void scheduleUpdateIfNeeded();
  bool isBuffered() const {
    return buffered;
  }
  bool shouldRealignShiftRegister();
  bool checkTransferWidth(uint32_t value);
public:
  Port();

  Signal getPinsValue() const;
  bool setCInUse(ThreadState &thread, bool val, ticks_t time);

  bool setCondition(ThreadState &thread, Condition c, ticks_t time);
  bool setData(ThreadState &thread, uint32_t d, ticks_t time);

  ResOpResult in(ThreadState &thread, ticks_t time, uint32_t &value);
  ResOpResult out(ThreadState &thread, uint32_t value, ticks_t time);
  ResOpResult outpw(ThreadState &thread, uint32_t value, uint32_t width,
                    ticks_t time);
  ResOpResult sync(ThreadState &thread, ticks_t time);
  ResOpResult setPortTime(ThreadState &thread, uint32_t value, ticks_t time);
  uint32_t getTimestamp(ThreadState &thread, ticks_t time);
  void clearPortTime(ThreadState &thread, ticks_t time);

  void connect(Port *p) { loopback = p; }
  
  unsigned getPortWidth() const
  {
    return getID().width();
  }

  unsigned getTransferWidth() const
  {
    return transferWidth;
  }

  uint32_t portWidthMask() const {
    return makeMask(getPortWidth());
  }

  uint32_t transferWidthMask() const {
    return makeMask(getTransferWidth());
  }

  void setClkInitial(ClockBlock *c) {
    clock = c;
    clock->attachPort(this);
    portCounter = 0;
    seeClockChange(time);
  }

  void setClk(ThreadState &thread, ClockBlock *c, ticks_t time);

  bool setReady(ThreadState &thread, Port *p, ticks_t time);

  bool setBuffered(ThreadState &thread, bool value, ticks_t time);

  bool setReadyMode(ThreadState &thread, ReadyMode readMode, ticks_t time);

  bool setMasterSlave(ThreadState &thread, MasterSlave value, ticks_t time);

  bool setPortType(ThreadState &thread, PortType portType, ticks_t time);

  bool setTransferWidth(ThreadState &thread, uint32_t value, ticks_t time);

  void seeClockStart(ticks_t time);

  void seeClockChange(ticks_t time);

  void clearBuf(ThreadState &thread, ticks_t time);

  void registerAsSourceOf(ClockBlock *c) {
    sourceOf.insert(c);
    scheduleUpdateIfNeeded();
  }

  void deregisterAsSourceOf(ClockBlock *c) {
    sourceOf.erase(c);
  }
  
  void registerAsReadyInOf(ClockBlock *c) {
    readyInOf.insert(c);
    scheduleUpdateIfNeeded();
  }
  
  void deregisterAsReadyInOf(ClockBlock *c) {
    readyInOf.erase(c);
  }

  void attachReadyOut(Port &p) {
    readyOutPorts.insert(&p);
  }

  void detachReadyOut(Port &p) {
    readyOutPorts.erase(&p);
  }

  /// Returns the number of rising edges before the first port width bits of the
  /// transfer register are output.
  unsigned calcEdgesBeforeTransferRegOutput();

  uint32_t nextShiftRegOutputPort(uint32_t old);

  void seeEdge(Edge::Type edgeType, ticks_t time);

  bool seeOwnerEventEnable();

  void skipEdges(unsigned numFalling, unsigned numRising);
  
  /// Update the port to the specified time.
  /// \param current The currently executing thread, or 0 if none.
  void update(ticks_t time);
  
  bool useReadyIn() const {
    return readyMode == HANDSHAKE ||
    (readyMode == STROBED && masterSlave == SLAVE);
  }

  bool useReadyOut() const {
    return readyMode == HANDSHAKE ||
    (readyMode == STROBED && masterSlave == MASTER);
  }

  bool nextReadyOut();

  void clearReadyOut(ticks_t time);
  void updateReadyOut(ticks_t time);

  virtual void completeEvent();

  void run(ticks_t time) {
    update(time);
    scheduleUpdateIfNeeded();
  }
protected:
  uint32_t getDataPortPinsValue(ticks_t time) const;
  Signal getDataPortPinsValue() const;
  bool seeEventEnable(ticks_t time);
};

#endif // _Port_h_
