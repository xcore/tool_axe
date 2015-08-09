// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Port_h_
#define _Port_h_

#include "Resource.h"
#include "BitManip.h"
#include "PortInterface.h"
#include "Signal.h"
#include <stdint.h>
#include <set>

namespace axe {

class Thread;
class ClockBlock;

class Port : public EventableResource, public PortInterface {
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
  PortInterface *loopback;
  PortInterface *tracer;
  /// Ready out ports.
  std::set<Port*> readyOutPorts;
  /// Thread paused on an output instruction.
  Thread *pausedOut;
  /// Thread paused on an input instruction.
  Thread *pausedIn;
  /// Thread paused on a sync instruction.
  Thread *pausedSync;
  /// Number of port width entries in the transfer register.
  unsigned shiftRegEntries;
  /// Number of port width entries for the current input / next output.
  unsigned portShiftCount;
  /// Number of valid port width entries in the transfer register.
  /// TODO rename to portShiftCount, change to alway count down to zero
  /// regardless of input / output.
  unsigned validShiftRegEntries;
  /// Shift register.
  uint32_t shiftReg;
  /// Transfer register.
  uint32_t transferReg;
  /// Port time register.
  uint16_t timeReg;
  /// Value of ready out on the last falling edge.
  bool readyOut;
  /// Value of ready in on the last sampling edge.
  bool readyIn;
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
  /// Is the port inverted?
  bool inverted;
  /// The edge that the port samples data.
  Edge::Type samplingEdge;
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
  /// Called whenever the value output to the pins changes.
  void outputValue(Signal value, ticks_t time);
  void outputValue(uint32_t value, ticks_t time) {
    return outputValue(Signal(value), time);
  }
  void handlePinsChange(Signal value, ticks_t time);
  /// Called whenever the value on the pins changes.
  void handlePinsChange(uint32_t value, ticks_t time);
  /// Called whenever the readyOut value changes.
  void handleReadyOutChange(bool value, ticks_t time);
  uint32_t computeSteadyStateInputShiftReg();
  /// Update the port to the specified time. The port must be clocked off a
  /// fixed frequency clock.
  void updateSlow(ticks_t time);

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
  unsigned fallingEdgesUntilTimeMet() const;
  /// Return the number of edges until the time is met. The time is always
  /// considered to be met on the falling edge at which the port counter is
  /// incremented.
  unsigned edgesUntilTimeMet() const;
  void scheduleUpdateIfNeededOutputPort();
  void scheduleUpdateIfNeededInputPort();
  void scheduleUpdateIfNeeded();
  bool isBuffered() const {
    return buffered;
  }
  bool isValidPortShiftCount(uint32_t count) const;
  bool shouldRealignShiftRegister();
  bool checkTransferWidth(uint32_t value);
  Signal getEffectiveValue(Signal value) const;
  uint32_t getEffectiveValue(uint32_t value) const {
    if (inverted)
      value ^= 1;
    return value;
  }
  Signal getPinsValue() const;
  Signal getDataPortPinsValue() const;
  Signal getEffectiveDataPortInputPinsValue() const {
    return getEffectiveValue(getDataPortPinsValue());
  }
  uint32_t getEffectiveDataPortInputPinsValue(ticks_t time) const {
    return getEffectiveDataPortInputPinsValue().getValue(time);
  }
  bool seeEventEnable(ticks_t time) override;
public:
  Port();
  std::string getName() const;
  Signal getEffectiveInputPinsValue() const {
    return getEffectiveValue(getPinsValue());
  }
  /// Update the pin buffer with the change.
  void seePinsChange(const Signal &value, ticks_t time) override;
  bool setCInUse(Thread &thread, bool val, ticks_t time) override;

  bool setCondition(Thread &thread, Condition c, ticks_t time) override;
  bool setData(Thread &thread, uint32_t d, ticks_t time) override;
  bool getData(Thread &thread, uint32_t &value, ticks_t time) override;
  bool setPortInv(Thread &thread, bool value, ticks_t time);
  void setSamplingEdge(Thread &thread, Edge::Type value, ticks_t time);
  bool setPinDelay(Thread &thread, unsigned value, ticks_t time);

  ResOpResult in(Thread &thread, ticks_t time, uint32_t &value) override;
  ResOpResult inpw(Thread &thread, uint32_t width, ticks_t time,
                   uint32_t &value);
  ResOpResult out(Thread &thread, uint32_t value, ticks_t time) override;
  ResOpResult outpw(Thread &thread, uint32_t value, uint32_t width,
                    ticks_t time);
  ResOpResult setpsc(Thread &thread, uint32_t value, ticks_t time);
  ResOpResult endin(Thread &thread, ticks_t time, uint32_t &value);
  ResOpResult sync(Thread &thread, ticks_t time);
  uint32_t peek(Thread &thread, ticks_t time);
  ResOpResult setPortTime(Thread &thread, uint32_t value, ticks_t time);
  uint32_t getTimestamp(Thread &thread, ticks_t time);
  void clearPortTime(Thread &thread, ticks_t time);

  PortInterface *getLoopback() const { return loopback; }
  void setLoopback(PortInterface *p) { loopback = p; }
  void setTracer(PortInterface *p) { tracer = p; }
  
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

  void setClkInitial(ClockBlock *c);

  void setClk(Thread &thread, ClockBlock *c, ticks_t time);

  bool setReady(Thread &thread, Port *p, ticks_t time) override;

  bool setBuffered(Thread &thread, bool value, ticks_t time);

  bool setReadyMode(Thread &thread, ReadyMode readMode, ticks_t time);

  bool setMasterSlave(Thread &thread, MasterSlave value, ticks_t time);

  bool setPortType(Thread &thread, PortType portType, ticks_t time);

  bool setTransferWidth(Thread &thread, uint32_t value, ticks_t time);

  void seeClockStart(ticks_t time);

  void seeClockChange(ticks_t time);

  void clearBuf(Thread &thread, ticks_t time);

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
  void seeEdge(EdgeIterator it) { seeEdge(it->type, it->time); }

  bool seeOwnerEventEnable();

  void updateNoExternalChange(unsigned numEdges);
  void updatePortCounter(unsigned numEdges);
  void updateInputValidShiftRegEntries(unsigned numEdges);

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

  bool computeReadyOut();
  bool readyOutIsInSteadyStateSlowPath();
  bool readyOutIsInSteadyState() {
    if (!useReadyOut())
      return !readyOut;
    return readyOutIsInSteadyStateSlowPath();
  }

  void clearReadyOut(ticks_t time);
  void updateReadyOut(ticks_t time);

  void completeEvent() override;

  void run(ticks_t time) override {
    update(time);
    scheduleUpdateIfNeeded();
  }
};
  
} // End axe namespace

#endif // _Port_h_
