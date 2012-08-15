// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _ClockBlock_h_
#define _ClockBlock_h_

#include "Resource.h"
#include "Signal.h"
#include <set>

class Port;

class ClockBlock : public Resource {
private:
  /// Clock source, 0 if source is reference clock.
  Port *source;
  /// Ready in port, 0 if no ready in.
  Port *readyIn;
  /// Clock divide
  unsigned divide;
  /// Attached ports
  std::set<Port*> ports;
  /// Current value.
  Signal value;
  /// Has the clock been started?
  bool running;
  Signal readyInValue;

  void updateAttachedPorts(ticks_t time);
  
  void seeChangeOnAttachedPorts(ticks_t time);
  
  void seeEdgeOnAttachedPorts(Edge::Type type, ticks_t time);
  
  Port *getSource() {
    return source;
  }
public:
  ClockBlock();
  
  bool setCInUse(Thread &thread, bool val, ticks_t time);

  bool setSource(Thread &thread, Port *p, ticks_t time);

  bool setEdgeDelay(Thread &thread, Edge::Type type, unsigned val,
                    ticks_t time);

  void setSourceRefClock(Thread &thread, ticks_t time);

  bool isFixedFrequency() const {
    return running && value.isClock();
  }

  bool setData(Thread &thread, uint32_t newDivide, ticks_t time);

  EdgeIterator getEdgeIterator(ticks_t time) const {
    return value.getEdgeIterator(time);
  }

#if (CYCLES_PER_TICK & 1) != 0
#error CYCLES_PER_TICK must be even
#endif
  uint32_t getHalfPeriod() {
    return divide * (CYCLES_PER_TICK / 2);
  }

  void attachPort(Port *port) {
    ports.insert(port);
  }

  void detachPort(Port *port) {
    ports.erase(port);
  }

  void setValue(const Signal &value, ticks_t time);
  Signal getValue() const;
  bool getValue(ticks_t time) const {
    return getValue().getValue(time) != 0;
  }

  bool setReady(Thread &thread, Port *p, ticks_t time);

  void start(Thread &thread, ticks_t time);
  void stop(Thread &thread, ticks_t time);
  void setReadyInValue(Signal value, ticks_t time);
  const Signal &getReadyInValue() const {
    return readyInValue;
  }
  uint32_t getReadyInValue(ticks_t time) const {
    return getReadyInValue().getValue(time);
  }
};

#endif //_ClockBlock_h_
