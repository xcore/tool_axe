// Copyright (c) 2014, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "SDRAM.h"
#include "Signal.h"
#include "RunnableQueue.h"
#include "PortInterface.h"
#include "Runnable.h"
#include "PortConnectionManager.h"
#include "PortHandleClockProxy.h"
#include "PortSignalTracker.h"
#include "PeripheralDescriptor.h"
#include "Peripheral.h"
#include "Property.h"
#include "SystemState.h"
#include "Node.h"
#include "Core.h"
#include "BitManip.h"
#include <queue>
#include <iostream>

using namespace axe;

struct SDRAMConfig {
  unsigned columnBits;
  unsigned rowBits;
  unsigned bankBits;
  uint32_t getNumEntries() const {
    return 1 << (columnBits + rowBits + bankBits);
  }
};

enum class SDRAMBustMode {
  SEQUENTIAL,
  INTERLEAVED
};

class SDRAMModeReg {
  uint16_t reg;
public:
  SDRAMModeReg() : reg(0) { }
  void set(uint16_t value) { reg = value; }
  unsigned getReadBurstLength() const;
  unsigned getWriteBurstLength() const;
  SDRAMBustMode getBurstMode() const;
  unsigned getCASLatency() const;
};

unsigned SDRAMModeReg::getReadBurstLength() const
{
  switch (reg & 0x7) {
  default:
    // Reserved, shouldn't happen.
    return 1;
  case 0:
    return 1;
  case 1:
    return 2;
  case 2:
    return 4;
  case 3:
    return 8;
  case 7:
    return 0; // Full page.
  }
}

unsigned SDRAMModeReg::getWriteBurstLength() const
{
  if (reg & (1 << 9))
    return 1;
  return getReadBurstLength();
}

SDRAMBustMode SDRAMModeReg::getBurstMode() const
{
  return (reg & (1 << 3)) ? SDRAMBustMode::INTERLEAVED :
                            SDRAMBustMode::SEQUENTIAL;
}

unsigned SDRAMModeReg::getCASLatency() const
{
  return (reg >> 4) & 0x7;
}

class SDRAM : public Peripheral {
  enum class Command {
    NOP,
    ACTIVE,
    READ,
    WRITE,
    BURST_TERMINATE,
    PRECHARGE,
    AUTO_REFRESH,
    LOAD_MODE_REGISTER,
  };
  std::vector<uint16_t> mem;
  std::queue<std::pair<uint16_t,ticks_t>> pendingReads;
  SDRAMConfig config;
  Command currentCommand;
  uint16_t currentColumn;
  uint16_t currentRow;
  uint32_t currentBank;
  uint16_t counter;
  uint8_t edgeNumber;
  SDRAMModeReg modeReg;
  PortSignalTracker A;
  PortSignalTracker BA;
  PortSignalTracker CAS;
  PortInterfaceMemberFuncDelegate<SDRAM> CLKProxy;
  PortHandleClockProxy CLKClock;
  PortSignalTracker DQ;
  PortInterface *DQPort;
  PortSignalTracker RAS;
  PortSignalTracker WE;
  bool useLDQM;
  PortSignalTracker LDQM;
  bool useUDQM;
  PortSignalTracker UDQM;
  uint32_t getMemoryIndex(unsigned burstLength) const;
  Command getCommand(ticks_t time) const;
  bool getLDQMValue(ticks_t time) const;
  bool getUDQMValue(ticks_t time) const;
  uint16_t getReadWriteMask(ticks_t time) const;
  void seeCLKChange(const Signal &value, ticks_t time);
public:
  SDRAM(RunnableQueue &scheduler, SDRAMConfig config);

  void connectA(PortConnectionWrapper p) { p.attach(&A); }
  void connectBA(PortConnectionWrapper p) { p.attach(&BA); }
  void connectCAS(PortConnectionWrapper p) { p.attach(&CAS); }
  void connectCLK(PortConnectionWrapper p) { p.attach(&CLKClock); }
  void connectDQ(PortConnectionWrapper p) {
    p.attach(&DQ);
    DQPort = p.getInterface();
  }
  void connectRAS(PortConnectionWrapper p) { p.attach(&RAS); }
  void connectWE(PortConnectionWrapper p) { p.attach(&WE); }
  void connectLDQM(PortConnectionWrapper p) { useLDQM = true; p.attach(&LDQM); }
  void connectUDQM(PortConnectionWrapper p) { useUDQM = true; p.attach(&UDQM); }
};

SDRAM::SDRAM(RunnableQueue &scheduler, SDRAMConfig config) :
  config(config),
  currentCommand(Command::NOP),
  edgeNumber(0),
  CLKProxy(*this, &SDRAM::seeCLKChange),
  CLKClock(scheduler, CLKProxy),
  DQPort(nullptr)
{
  mem.resize(config.getNumEntries());
}

uint32_t SDRAM::getMemoryIndex(unsigned burstLength) const
{
  uint16_t column;
  if (burstLength == 0) {
    column = (currentColumn + counter) & ((1 << config.columnBits) - 1);
  } else {
    column = currentColumn;
    switch (modeReg.getBurstMode()) {
    case SDRAMBustMode::INTERLEAVED:
      column = currentColumn ^ counter;
      break;
    case SDRAMBustMode::SEQUENTIAL:
      column = (currentColumn / burstLength) * burstLength;
      column += (currentColumn + counter) % burstLength;
      if (column / burstLength > currentColumn / burstLength)
        column -= burstLength;
      break;
    }
  }
  uint32_t index = 0;
  index |= column;
  index |= currentRow << config.columnBits;
  index |= currentBank << (config.columnBits + config.rowBits);
  assert(index < config.getNumEntries());
  return index;
}

SDRAM::Command SDRAM::getCommand(ticks_t time) const
{
  uint32_t cmd = 0;
  if (WE.getSignal().getValue(time))
    cmd |= 0x1;
  if (CAS.getSignal().getValue(time))
    cmd |= 0x2;
  if (RAS.getSignal().getValue(time))
    cmd |= 0x4;
  switch (cmd) {
  default:
    assert(0 && "Unexpected cmd");
  case 0:
    return Command::LOAD_MODE_REGISTER;
  case 1:
    return Command::AUTO_REFRESH;
  case 2:
    return Command::PRECHARGE;
  case 3:
    return Command::ACTIVE;
  case 4:
    return Command::WRITE;
  case 5:
    return Command::READ;
  case 6:
    return Command::BURST_TERMINATE;
  case 7:
    return Command::NOP;
  }
}

bool SDRAM::getLDQMValue(ticks_t time) const
{
  if (useLDQM)
    LDQM.getSignal().getValue(time);
  // Otherwise use NOR of CAS and RAS signal to match the SDRAM slice.
  return CAS.getSignal().getValue(time) == 0 &&
         RAS.getSignal().getValue(time) == 0;
}

bool SDRAM::getUDQMValue(ticks_t time) const
{
  if (useUDQM)
    return UDQM.getSignal().getValue(time);
  // Otherwise use NOR of CAS and RAS signal to match the SDRAM slice.
  return CAS.getSignal().getValue(time) == 0 &&
         RAS.getSignal().getValue(time) == 0;
}

uint16_t SDRAM::getReadWriteMask(ticks_t time) const
{
  uint16_t mask = 0;
  if (getLDQMValue(time) == 0)
    mask |= 0x00ff;
  if (getUDQMValue(time) == 0)
    mask |= 0xff00;
  return mask;
}

void SDRAM::seeCLKChange(const Signal &value, ticks_t time)
{
  if (value.getValue(time)) {
    // Rising edge.
    Command newCommand = getCommand(time);
    switch (newCommand) {
    case Command::ACTIVE:
      currentRow = A.getSignal().getValue(time) & ((1 << config.rowBits) - 1);
      break;
    case Command::AUTO_REFRESH:
    case Command::NOP:
    case Command::PRECHARGE:
      break;
    case Command::LOAD_MODE_REGISTER:
      modeReg.set(A.getSignal().getValue(time));
      break;
    case Command::READ:
      currentColumn =
        A.getSignal().getValue(time) & ((1 << config.columnBits) - 1);
      currentBank =
        BA.getSignal().getValue(time) & ((1 << config.bankBits) - 1);
      counter = 0;
      break;
    case Command::WRITE:
      currentColumn =
        A.getSignal().getValue(time) & ((1 << config.columnBits) - 1);
      currentBank =
        BA.getSignal().getValue(time) & ((1 << config.bankBits) - 1);
      counter = 0;
      break;
    case Command::BURST_TERMINATE:
      break;
    }
    if (newCommand != Command::NOP) {
      currentCommand = newCommand;
    }
    switch (currentCommand) {
    default:
      break;
    case Command::READ:
      {
        uint16_t mask = getReadWriteMask(time);
        unsigned burstLength = modeReg.getReadBurstLength();
        uint16_t data = mem[getMemoryIndex(burstLength)] & mask;
        uint8_t readEdge = edgeNumber + modeReg.getCASLatency();
        pendingReads.push(std::make_pair(data, readEdge));
        ++counter;
        if (burstLength != 0 && counter == burstLength)
          currentCommand = Command::NOP;
      }
      break;
    case Command::WRITE:
      {
        unsigned burstLength = modeReg.getReadBurstLength();
        if (WE.getSignal().getValue(time)) {
          uint16_t mask = getReadWriteMask(time);
          uint32_t index = getMemoryIndex(burstLength);
          uint16_t old = mem[index];
          uint16_t data = DQ.getSignal().getValue(time);
          mem[index] ^= (old ^ data) & mask;
        }
        ++counter;
        if (burstLength != 0 && counter == burstLength)
          currentCommand = Command::NOP;
      }
      break;
    }
    ++edgeNumber;
  } else {
    // Falling edge.
    if (!pendingReads.empty() && pendingReads.front().second == edgeNumber) {
      DQPort->seePinsChange(Signal(pendingReads.front().first), time);
      pendingReads.pop();
    }
  }
}

static Peripheral *
createSDRAM(SystemState &system, PortConnectionManager &connectionManager,
            const Properties &properties)
{
  SDRAMConfig config;
  config.columnBits = 8;
  config.rowBits = 12;
  config.bankBits = 2;
  auto p = new SDRAM(system.getScheduler(), config);
  p->connectA(connectionManager.get(properties, "a"));
  p->connectBA(connectionManager.get(properties, "ba"));
  p->connectCAS(connectionManager.get(properties, "cas"));
  p->connectCLK(connectionManager.get(properties, "clk"));
  p->connectDQ(connectionManager.get(properties, "dq"));
  p->connectRAS(connectionManager.get(properties, "ras"));
  p->connectWE(connectionManager.get(properties, "we"));
  if (properties.get("ldqm"))
    p->connectLDQM(connectionManager.get(properties, "ldqm"));
  if (properties.get("udqm"))
    p->connectUDQM(connectionManager.get(properties, "udqm"));
  return p;
}

std::auto_ptr<PeripheralDescriptor> axe::getPeripheralDescriptorSDRAM()
{
  std::auto_ptr<PeripheralDescriptor> p(
    new PeripheralDescriptor("sdram", &createSDRAM));
  p->addProperty(PropertyDescriptor::portProperty("a")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("ba")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("cas")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("cke"));
  p->addProperty(PropertyDescriptor::portProperty("clk")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("cs"));
  p->addProperty(PropertyDescriptor::portProperty("dq")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("ldqm"));
  p->addProperty(PropertyDescriptor::portProperty("udqm"));
  p->addProperty(PropertyDescriptor::portProperty("ras")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("we")).setRequired(true);
  return p;
}
