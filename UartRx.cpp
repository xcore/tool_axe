// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "UartRx.h"
#include "Signal.h"
#include "RunnableQueue.h"
#include <iostream>

// TODO use more appropriate type than EVENTABLE_RESOURCE.
UartRx::UartRx(RunnableQueue &s) :
  Runnable(EVENTABLE_RESOURCE),
  scheduler(s),
  currentPinValue(0),
  bitTime((CYCLES_PER_TICK*100000000)/115200),
  numDataBits(8),
  numStopBits(1),
  parity(NONE),
  state(WAIT_FOR_HIGH)
{
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
