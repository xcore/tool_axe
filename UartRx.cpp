// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "UartRx.h"
#include "Signal.h"
#include "RunnableQueue.h"
#include "PortInterface.h"
#include "Runnable.h"
#include "PortConnectionManager.h"
#include "PeripheralDescriptor.h"
#include "Peripheral.h"
#include "Property.h"
#include "SystemState.h"
#include "Node.h"
#include "Core.h"
#include <iostream>

using namespace axe;

const unsigned DEFAULT_BIT_RATE = 115200;

class UartRx : public PortInterface, public Runnable, public Peripheral {
private:
  RunnableQueue &scheduler;
  unsigned currentPinValue;
  enum State {
    WAIT_FOR_HIGH,
    WAIT_FOR_START,
    CHECK_START,
    RECEIVE,
    CHECK_PARITY,
    CHECK_STOP
  };
  ticks_t bitTime;
  unsigned numDataBits;
  unsigned numStopBits;
  enum Parity {
    ODD,
    EVEN,
    NONE
  };
  Parity parity;
  State state;
  bool byteValid;
  unsigned bitNumber;
  unsigned byte;
  
  bool computeParity();
  void receiveByte(unsigned byte);
  void scheduleUpdate(ticks_t time);
public:
  UartRx(RunnableQueue &s, PortConnectionWrapper p, ticks_t bitTime);
  virtual void seePinsChange(const Signal &value, ticks_t time);
  virtual void run(ticks_t time);
};

UartRx::UartRx(RunnableQueue &s, PortConnectionWrapper p, ticks_t bt) :
  scheduler(s),
  currentPinValue(0),
  bitTime(bt),
  numDataBits(8),
  numStopBits(1),
  parity(NONE),
  state(WAIT_FOR_HIGH)
{
  p.attach(this);
}

void UartRx::scheduleUpdate(ticks_t time)
{
  scheduler.push(*this, time);
}

void UartRx::seePinsChange(const Signal &value, ticks_t time)
{
  currentPinValue = value.getValue(time);
  switch (state) {
  default:
    break;
  case WAIT_FOR_HIGH:
    if (currentPinValue == 1)
      state = WAIT_FOR_START;
    break;
  case WAIT_FOR_START:
    if (currentPinValue == 0) {
      state = CHECK_START;
      scheduleUpdate(time + bitTime / 2);
    }
    break;
  }
}

bool UartRx::computeParity()
{
  bool value = 0;
  unsigned tmp = byte;
  if (parity == ODD)
    value = !value;
  while (tmp) {
    value = !value;
    tmp = tmp & (tmp - 1);
  }
  return value;
}

void UartRx::receiveByte(unsigned byte)
{
  std::cout << (char)byte;
  std::cout.flush();
}

void UartRx::run(ticks_t time)
{
  switch (state) {
  default: assert(0 && "Unexpected state");
  case CHECK_START:
    if (currentPinValue == 0) {
      state = RECEIVE;
      bitNumber = 0;
      byte = 0;
      byteValid = true;
      scheduleUpdate(time + bitTime);
    } else {
      state = WAIT_FOR_START;
    }
    break;
  case RECEIVE:
    byte |= currentPinValue << bitNumber;
    bitNumber++;
    if (bitNumber == numDataBits) {
      state = (parity == NONE) ? CHECK_STOP : CHECK_PARITY;
      bitNumber = 0;
    }
    scheduleUpdate(time + bitTime);
    break;
  case CHECK_PARITY:
    byteValid = currentPinValue == computeParity();
    state = CHECK_STOP;
    scheduleUpdate(time + bitTime);
    break;
  case CHECK_STOP:
    if (currentPinValue == 1) {
      bitNumber++;
      if (bitNumber == numStopBits) {
        if (byteValid)
        state = WAIT_FOR_START;
        receiveByte(byte);
      } else {
        scheduleUpdate(time + bitTime);
      }
    } else {
      state = WAIT_FOR_HIGH;
    }
    break;
  }
}

static Peripheral *
createUartRx(SystemState &system, PortConnectionManager &connectionManager,
             const Properties &properties)
{
  int32_t bitrate = DEFAULT_BIT_RATE;
  if (const Property *p = properties.get("bitrate"))
    bitrate =  p->getAsInteger();
  ticks_t bitTime = (CYCLES_PER_TICK*100000000)/bitrate;
  PortConnectionWrapper port = connectionManager.get(properties, "port");
  UartRx *p =
    new UartRx(system.getScheduler(), port, bitTime);
  return p;
}

std::auto_ptr<PeripheralDescriptor> axe::getPeripheralDescriptorUartRx()
{
  std::auto_ptr<PeripheralDescriptor> p(
    new PeripheralDescriptor("uart-rx", &createUartRx));
  p->addProperty(PropertyDescriptor::portProperty("port")).setRequired(true);
  p->addProperty(PropertyDescriptor::integerProperty("bitrate"));
  return p;
}
