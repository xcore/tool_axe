// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "SPIFlash.h"
#include "Signal.h"
#include "RunnableQueue.h"
#include "PortInterface.h"
#include "Runnable.h"
#include "PeripheralDescriptor.h"
#include "Peripheral.h"
#include "Property.h"
#include "SystemState.h"
#include "Node.h"
#include "Core.h"
#include "PortHandleClockProxy.h"
#include <fstream>
#include <iostream>
#include <cstdlib>

class SPIFlash : public Peripheral {
  void seeMOSIChange(const Signal &value, ticks_t time);
  void seeSCLKChange(const Signal &value, ticks_t time);
  void seeSSChange(const Signal &value, ticks_t time);
  enum State {
    WAIT_FOR_CMD,
    WAIT_FOR_ADDRESS,
    READ,
    UNKNOWN_CMD
  };
  State state;
  unsigned char *mem;
  unsigned memSize;
  Port *MISO;
  PortInterfaceMemberFuncDelegate<SPIFlash> MOSIProxy;
  PortInterfaceMemberFuncDelegate<SPIFlash> SCLKProxy;
  PortInterfaceMemberFuncDelegate<SPIFlash> SSProxy;
  PortHandleClockProxy SCLKHandleClock;
  PortHandleClockProxy SSHandleClock;
  unsigned MISOValue;
  Signal MOSIValue;
  unsigned SCLKValue;
  unsigned SSValue;
  uint8_t receiveReg;
  unsigned receivedBits;
  unsigned receivedAddressBytes;
  uint8_t sendReg;
  unsigned sendBitsRemaining;
  uint32_t readAddress;

  void reset();
public:
  SPIFlash(RunnableQueue &scheduler, Port *MISO);
  ~SPIFlash();
  void openFile(const std::string &s);
  void connectMOSI(Port *p) { p->setLoopback(&MOSIProxy); }
  void connectSCLK(Port *p) { p->setLoopback(&SCLKHandleClock); }
  void connectSS(Port *p) { p->setLoopback(&SSHandleClock); }
};

SPIFlash::SPIFlash(RunnableQueue &scheduler, Port *p) :
  mem(0),
  memSize(0),
  MISO(p),
  MOSIProxy(*this, &SPIFlash::seeMOSIChange),
  SCLKProxy(*this, &SPIFlash::seeSCLKChange),
  SSProxy(*this, &SPIFlash::seeSSChange),
  SCLKHandleClock(scheduler, SCLKProxy),
  SSHandleClock(scheduler, SSProxy),
  MISOValue(0),
  MOSIValue(0),
  SCLKValue(0),
  SSValue(0)
{
  reset();
}

SPIFlash::~SPIFlash()
{
  delete[] mem;
}

void SPIFlash::reset()
{
  state = WAIT_FOR_CMD;
  receiveReg = 0;
  receivedBits = 0;
  receivedAddressBytes = 0;
  readAddress = 0;
  sendReg = 0;
  sendBitsRemaining = 0;
}

void SPIFlash::seeMOSIChange(const Signal &value, ticks_t time)
{
  MOSIValue = value;
}

void SPIFlash::seeSCLKChange(const Signal &value, ticks_t time)
{
  unsigned newValue = value.getValue(time);
  if (newValue == SCLKValue)
    return;
  SCLKValue = newValue;
  if (SSValue != 0)
    return;
  if (SCLKValue == 1) {
    // Rising edge.
    receiveReg = (receiveReg << 1) | MOSIValue.getValue(time);
    if (++receivedBits == 8) {
      switch (state) {
      default: assert(0 && "Unexpected state");
      case WAIT_FOR_CMD:
        if (receiveReg == 0x3)
          state = WAIT_FOR_ADDRESS;
        else
          state = UNKNOWN_CMD;
        break;
      case WAIT_FOR_ADDRESS:
        readAddress = (readAddress << 8) | receiveReg;
        if (++receivedAddressBytes == 3) {
          state = READ;
        }
      case READ:
      case UNKNOWN_CMD:
        // Do nothing.
        break;
      }
      receiveReg = 0;
      receivedBits = 0;
    }
  } else {
    // Falling edge.
    if (state == READ) {
      // Output
      if (mem && sendBitsRemaining == 0) {
        sendReg = mem[readAddress++ % memSize];
        sendBitsRemaining = 8;
      }
      unsigned newValue = (sendReg >> 7) & 1;
      if (newValue != MISOValue) {
        MISO->seePinsChange(Signal(newValue), time);
        MISOValue = newValue;
      }
      sendReg <<= 1;
      --sendBitsRemaining;
    }
  }
}

void SPIFlash::seeSSChange(const Signal &value, ticks_t time)
{
  unsigned newValue = value.getValue(time);
  if (newValue == SSValue)
    return;
  SSValue = newValue;
  if (SSValue == 1)
    reset();
}

void SPIFlash::openFile(const std::string &s)
{
  delete[] mem;
  std::ifstream file(s.c_str(),
                     std::ios::in|std::ios::binary|std::ios::ate);
  if (!file) {
    std::cerr << "Error opening \"" << s << "\"\n";
    std::exit(1);
  }
  memSize = file.tellg();
  mem = new uint8_t[memSize];
  file.seekg(0, std::ios::beg);
  file.read(reinterpret_cast<char*>(mem), memSize);
  if (!file) {
    std::cerr << "Error reading \"" << s << "\"\n";
    std::exit(1);
  }
  file.close();
}

static Peripheral *
createSPIFlash(SystemState &system, const Properties &properties)
{
  Port *MISO = properties.get("miso")->getAsPort().lookup(system);
  Port *MOSI = properties.get("mosi")->getAsPort().lookup(system);
  Port *SCLK = properties.get("sclk")->getAsPort().lookup(system);
  Port *SS = properties.get("ss")->getAsPort().lookup(system);
  std::string file = properties.get("filename")->getAsString();
  SPIFlash *p = new SPIFlash(system.getScheduler(), MISO);
  p->openFile(file);
  p->connectMOSI(MOSI);
  p->connectSCLK(SCLK);
  p->connectSS(SS);
  return p;
}

std::auto_ptr<PeripheralDescriptor> getPeripheralDescriptorSPIFlash()
{
  std::auto_ptr<PeripheralDescriptor> p(
    new PeripheralDescriptor("spi-flash", &createSPIFlash));
  p->addProperty(PropertyDescriptor::portProperty("miso")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("mosi")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("sclk")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("ss")).setRequired(true);
  p->addProperty(PropertyDescriptor::stringProperty("filename"))
    .setRequired(true);
  return p;
}
