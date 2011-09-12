// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>


#ifndef _Uart_h_
#define _Uart_h_

#include "PortInterface.h"
#include "Runnable.h"

class RunnableQueue;

class UartRx : public PortInterface, public Runnable {
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
  UartRx(RunnableQueue &s);
  virtual void seePinsChange(const Signal &value, ticks_t time);
  virtual void run(ticks_t time);
};

#endif // _Uart_h_
