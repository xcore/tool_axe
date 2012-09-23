// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "EthernetPhy.h"
#include "Peripheral.h"
#include "PortInterface.h"
#include "Port.h"
#include "PortHandleClockProxy.h"
#include "SystemState.h"
#include "Property.h"
#include "Config.h"
#include "NetworkLink.h"
#include "CRC.h"
#include "BitManip.h"
#include <memory>
#include <cstring>

// MII uses a 25MHz clock.
const uint32_t ethernetPhyHalfPeriod = CYCLES_PER_TICK * 2;
const uint32_t ethernetPhyPeriod = ethernetPhyHalfPeriod * 2;
const unsigned interframeGap = (12 * 8) / 4; // Time to transmit 12 bytes.
const uint32_t CRC32Poly = 0xEDB88320;
/// Minimum frame size (including CRC).
const unsigned minFrameSize = 64;

class EthernetPhyTx : public Runnable {
  void seeTXDChange(const Signal &value, ticks_t time);
  void seeTX_ENChange(const Signal &value, ticks_t time);
  void seeTX_ERChange(const Signal &value, ticks_t time);

  RunnableQueue &scheduler;
  NetworkLink *link;
  Port *TX_CLK;
  PortInterfaceMemberFuncDelegate<EthernetPhyTx> TXDProxy;
  PortInterfaceMemberFuncDelegate<EthernetPhyTx> TX_ENProxy;
  PortInterfaceMemberFuncDelegate<EthernetPhyTx> TX_ERProxy;
  Signal TX_CLKSignal;
  Signal TXDSignal;
  Signal TX_ENSignal;
  Signal TX_ERSignal;

  bool inFrame;
  bool hadError;
  std::vector<uint8_t> frame;
  bool hasPrevNibble;
  uint8_t prevNibble;

  void reset();
  bool transmitFrame();
  bool possibleSFD();
public:
  EthernetPhyTx(RunnableQueue &sched, Port *MISO);
  void setLink(NetworkLink *value) { link = value; }
  void connectTXD(Port *p) { p->setLoopback(&TXDProxy); }
  void connectTX_EN(Port *p) { p->setLoopback(&TX_ENProxy); }
  void connectTX_ER(Port *p) { p->setLoopback(&TX_ERProxy); }
  void run(ticks_t time);
};

EthernetPhyTx::EthernetPhyTx(RunnableQueue &s, Port *txclk) :
  scheduler(s),
  link(0),
  TX_CLK(txclk),
  TXDProxy(*this, &EthernetPhyTx::seeTXDChange),
  TX_ENProxy(*this, &EthernetPhyTx::seeTX_ENChange),
  TX_ERProxy(*this, &EthernetPhyTx::seeTX_ERChange),
  TX_CLKSignal(0, ethernetPhyHalfPeriod),
  TXDSignal(0),
  TX_ENSignal(0),
  TX_ERSignal(0)
{
  reset();
  TX_CLK->seePinsChange(TX_CLKSignal, 0);
}

void EthernetPhyTx::reset()
{
  inFrame = false;
  hadError = false;
  hasPrevNibble = false;
  frame.clear();
}

bool EthernetPhyTx::transmitFrame()
{
  if (frame.size() < minFrameSize)
    return false;

  // Check CRC.
  uint32_t crc = 0x9226F562;
  for (unsigned i = 0, e = frame.size(); i != e; ++i) {
    crc = crc8(crc, frame[i], CRC32Poly);
  }
  crc = ~crc;
  if (crc != 0) {
    return false;
  }
  
  link->transmitFrame(&frame[0], frame.size() - 4);
  return true;
}

bool EthernetPhyTx::possibleSFD()
{
  if (TX_ENSignal.isClock() || TX_ENSignal.getValue(0) == 1) {
    return TXDSignal.getValue(0) == 0xd;
  }
  return false;
}

void EthernetPhyTx::seeTXDChange(const Signal &value, ticks_t time)
{
  TXDSignal = value;
  if (!inFrame && possibleSFD()) {
    Edge nextRisingEdge = TX_CLKSignal.getNextEdge(time - 1, Edge::RISING);
    scheduler.push(*this, nextRisingEdge.time);
  }
}

void EthernetPhyTx::seeTX_ENChange(const Signal &value, ticks_t time)
{
  TX_ENSignal = value;
  if (!inFrame && possibleSFD()) {
    Edge nextRisingEdge = TX_CLKSignal.getNextEdge(time - 1, Edge::RISING);
    scheduler.push(*this, nextRisingEdge.time);
  }
}

void EthernetPhyTx::seeTX_ERChange(const Signal &value, ticks_t time)
{
  TX_ERSignal = value;
}

void EthernetPhyTx::run(ticks_t time)
{
  // Sample on rising edge.
  uint32_t TXDValue = TXDSignal.getValue(time);
  uint32_t TX_ENValue = TX_ENSignal.getValue(time);
  uint32_t TX_ERValue = TX_ERSignal.getValue(time);
  if (inFrame) {
    if (TX_ENValue) {
      if (TX_ERValue) {
        hadError = true;
      } else {
        // Receive data
        if (hasPrevNibble) {
          frame.push_back((TXDValue << 4) | prevNibble);
          hasPrevNibble = false;
        } else {
          prevNibble = TXDValue;
          hasPrevNibble = true;
        }
      }
    } else {
      // End of frame.
      if (!hadError) {
        transmitFrame();
      }
      reset();
    }
  } else {
    // Look for the second Start Frame Delimiter nibble.
    // TODO should we be checking for the preamble before this as well?
    // sc_ethernet seems to emit a longer preamble than is specified in the
    // 802.3 standard (12 octets including SFD instead of 8).
    if (TX_ENValue && TXDValue == 0xd) {
      inFrame = true;
      hadError = TX_ERValue;
    }
  }
  if (TX_ENValue == 1 || TX_ENSignal.isClock()) {
    scheduler.push(*this, time + ethernetPhyPeriod);
  }
}

class EthernetPhyRx : public Runnable {
  RunnableQueue &scheduler;
  NetworkLink *link;
  Port *RX_CLK;
  Port *RXD;
  Port *RX_DV;
  Port *RX_ER;
  Signal RX_CLKSignal;
  uint32_t RXDValue;
  uint8_t frame[NetworkLink::maxFrameSize];
  unsigned frameSize;
  unsigned nibblesReceieved;
  enum State {
    IDLE,
    TX_SFD2,
    TX_FRAME,
    TX_EFD,
  } state;
  void appendCRC32();
  bool receiveFrame();
  void setRXD(unsigned value, ticks_t time);
public:
  EthernetPhyRx(RunnableQueue &s, Port *rxclk, Port *rxd, Port *rxdv,
                Port *rxer);

  void setLink(NetworkLink *value) { link = value; }
  void run(ticks_t time);
};

EthernetPhyRx::
EthernetPhyRx(RunnableQueue &s, Port *rxclk, Port *rxd, Port *rxdv,
              Port *rxer) :
  scheduler(s),
  link(0),
  RX_CLK(rxclk),
  RXD(rxd),
  RX_DV(rxdv),
  RX_ER(rxer),
  RX_CLKSignal(0, ethernetPhyHalfPeriod),
  RXDValue(0),
  state(IDLE)
{
  RX_CLK->seePinsChange(RX_CLKSignal, 0);
  ticks_t fallingEdgeTime = RX_CLKSignal.getNextEdge(0, Edge::FALLING).time;
  scheduler.push(*this, fallingEdgeTime);
}

void EthernetPhyRx::appendCRC32()
{
  // Initializing the CRC with 0x9226F562 is equivalent to initializing the CRC
  // with 0 and inverting the first 4 bytes.
  uint32_t crc = 0x9226F562;
  for (unsigned i = 0; i < frameSize; i++) {
    crc = crc8(crc, frame[i], CRC32Poly);
  }
  crc = crc32(crc, 0, CRC32Poly);
  crc = ~crc;
  frame[frameSize] = crc;
  frame[frameSize + 1] = crc >> 8;
  frame[frameSize + 2] = crc >> 16;
  frame[frameSize + 3] = crc >> 24;
  frameSize += 4;
}

bool EthernetPhyRx::receiveFrame()
{
  if (!link->receiveFrame(frame, frameSize))
    return false;
  const unsigned minSize = minFrameSize - 4;
  if (frameSize < minSize) {
    std::memset(&frame[frameSize], 0, minSize - frameSize);
    frameSize = minSize;
  }
  appendCRC32();
  return true;
}

void EthernetPhyRx::setRXD(unsigned value, ticks_t time)
{
  if (value == RXDValue)
    return;
  RXD->seePinsChange(Signal(value), time);
  RXDValue = value;
}

void EthernetPhyRx::run(ticks_t time)
{
  ticks_t nextTime = time + ethernetPhyPeriod;
  // Drive on falling edge.
  switch (state) {
  case IDLE:
    if (receiveFrame()) {
      setRXD(0x5, time);
      RX_DV->seePinsChange(Signal(1), time);
      state = TX_SFD2;
    }
    break;
  case TX_SFD2:
    setRXD(0xd, time);
    nibblesReceieved = 0;
    state = TX_FRAME;
    break;
  case TX_FRAME:
    {
      unsigned byteNum = nibblesReceieved / 2;
      unsigned nibbleNum = nibblesReceieved % 2;
      unsigned data = (frame[byteNum] >> (nibbleNum * 4)) & 0xff;
      setRXD(data, time);
      ++nibblesReceieved;
      if (nibblesReceieved == frameSize * 2) {
        state = TX_EFD;
      }
    }
    break;
  case TX_EFD:
    setRXD(0, time);
    RX_DV->seePinsChange(Signal(0), time);
    state = IDLE;
    nextTime = time + ethernetPhyPeriod * interframeGap;
    break;
  }
  scheduler.push(*this, nextTime);
}

namespace Status {
  enum {
    EXTENDED_CAPABILLITY = 1 << 0,
    JABBER_DETECT = 1 << 1,
    LINK_STATUS = 1 << 2,
    AUTO_NEGOTIATION_ABILITY = 1 << 3,
    REMOTE_FAULT = 1 << 4,
    AUTO_NEGOTIATION_COMPLETE = 1 << 5,
    MF_PREAMBLE_SUPRESSION = 1 << 6,
    UNIDIRECTIONAL_ABILITY = 1 << 7,
    EXTENDED_STATUS = 1 << 8,
    SUPPORTS_100BASE_T2_HALF_DUPLEX = 1 << 9,
    SUPPORTS_100BASE_T2_FULL_DUPLEX = 1 << 10,
    SUPPORTS_10_MBS_HALF_DUPLEX = 1 << 11,
    SUPPORTS_10_MBS_FULL_DUPLEX = 1 << 12,
    SUPPORTS_100BASE_X_HALF_DUPLEX = 1 << 13,
    SUPPORTS_100BASE_X_FULL_DUPLEX = 1 << 14,
    SUPPORTS_100BASE_T4 = 1 << 15
  };
}

class EthernetPhyRegisters {
  // Basic register set, as defined by 802.3 22.2.4
  enum {
    CONTROL = 0,
    STATUS = 1
  };
  uint16_t readStatus();
public:
  void write(unsigned reg, uint16_t value);
  uint16_t read(unsigned reg);
};

void EthernetPhyRegisters::write(unsigned reg, uint16_t value)
{
  switch (reg) {
  default:
    break;
  case CONTROL:
    // TODO
    break;
  }
}

uint16_t EthernetPhyRegisters::readStatus()
{
  uint16_t status = 0;
  status |= Status::LINK_STATUS;
  // TODO other status bits.
  return status;
}

uint16_t EthernetPhyRegisters::read(unsigned reg)
{
  switch (reg) {
  default:
    return 0;
  case CONTROL:
    // TODO
    return 0;
    break;
  case STATUS:
    return readStatus();
  }
}

class EthernetPhySMI {
  Port *MDIO;
  unsigned phyAddress;

  bool outputMode;
  Signal MDIOSignal;
  uint32_t shiftReg;
  unsigned shiftCount;
  unsigned regNum;
  EthernetPhyRegisters registers;

  enum {
    READ_OP = 2,
    WRITE_OP = 1
  };

  enum State {
    WAIT_FOR_PREAMBLE,
    WAIT_FOR_FRAME_START,
    FRAME_START,
    WRITE,
    READ1,
    READ2,
    READ3
  } state;
  PortInterfaceMemberFuncDelegate<EthernetPhySMI> MDCProxy;
  PortInterfaceMemberFuncDelegate<EthernetPhySMI> MDIOProxy;
  PortHandleClockProxy MDCHandleClock;
  PortHandleClockProxy MDIOHandleClock;
public:
  EthernetPhySMI(RunnableQueue &scheduler, Port *mdc, Port *mdio);
  void seeMDCChange(const Signal &value, ticks_t time);
  void seeMDIOChange(const Signal &value, ticks_t time);
};

void EthernetPhySMI::seeMDCChange(const Signal &value, ticks_t time)
{
  if (value.getValue(time) && !outputMode) {
    // Sample on rising edge.
    unsigned MDIOValue = MDIOSignal.getValue(time);
    shiftReg = (shiftReg << 1) | MDIOValue;
    switch (state) {
    default:
      assert(0 && "Unexpected state");
    case WAIT_FOR_PREAMBLE:
      if (shiftReg == 0xffffffff)
        state = WAIT_FOR_FRAME_START;
      break;
    case WAIT_FOR_FRAME_START:
      if (MDIOValue == 0) {
        shiftCount = 1;
        state = FRAME_START;
      }
      break;
    case FRAME_START:
      if (++shiftCount == 2 + 2 + 5 + 5) {
        // Extract
        regNum = extractBits(shiftReg, 0, 5);
        unsigned phy =extractBits(shiftReg, 5, 5);
        unsigned operation = extractBits(shiftReg, 10, 2);
        unsigned startOfFrame = extractBits(shiftReg, 12, 2);
        if (startOfFrame != 1) {
          state = WAIT_FOR_PREAMBLE;
          break;
        }
        if (phy != 0 && phy != phyAddress) {
          state = WAIT_FOR_PREAMBLE;
          break;
        }
        switch (operation) {
        default:
          // Unknown operation.
          state = WAIT_FOR_PREAMBLE;
          break;
        case 1:
          // Write.
          shiftCount = 0;
          state = WRITE;
          break;
        case 2:
          // Read.
          state = READ1;
          break;
        }
      }
      break;
    case WRITE:
      if (++shiftCount == 16 + 2) {
        registers.write(regNum, extractBits(shiftReg, 0, 16));
        state = WAIT_FOR_PREAMBLE;
      }
      break;
    case READ1:
      // Switch to output on next falling edge.
      outputMode = true;
      state = READ2;
      break;
    }
  } else if (value.getValue(time) == 0 && outputMode) {
    // Drive on falling edge.
    switch (state) {
    default:
      assert(0 && "Unexpected state");
    case READ2:
      // Drive zero on second falling edge.
      MDIO->seePinsChange(Signal(0), time);
      shiftReg = registers.read(regNum);
      shiftCount = 0;
      state = READ3;
      break;
    case READ3:
      {
        if (shiftCount == 16) {
          // Switch to input.
          outputMode = false;
          state = WAIT_FOR_PREAMBLE;
          shiftReg = 0;
        }
        MDIO->seePinsChange(Signal(extractBit(shiftReg, 16)), time);
        shiftReg <<= 1;
        ++shiftCount;
      }
    }
  }
}

void EthernetPhySMI::seeMDIOChange(const Signal &value, ticks_t time)
{
  MDIOSignal = value;
}

EthernetPhySMI::
EthernetPhySMI(RunnableQueue &scheduler, Port *mdc, Port *mdio) :
  MDIO(mdio),
  outputMode(false),
  shiftReg(0),
  state(WAIT_FOR_PREAMBLE),
  MDCProxy(*this, &EthernetPhySMI::seeMDCChange),
  MDIOProxy(*this, &EthernetPhySMI::seeMDIOChange),
  MDCHandleClock(scheduler, MDCProxy),
  MDIOHandleClock(scheduler, MDIOProxy)
{
  mdc->setLoopback(&MDCHandleClock);
  mdio->setLoopback(&MDIOHandleClock);
}

class EthernetPhy : public Peripheral {
  std::auto_ptr<NetworkLink> link;
  EthernetPhyRx rx;
  EthernetPhyTx tx;
  EthernetPhySMI smi;
public:
  EthernetPhy(RunnableQueue &s, Port *TX_CLK, Port *RX_CLK, Port *RXD,
              Port *RX_DV, Port *RX_ER, Port *MDC, Port *MDIO,
              const std::string &ifname);
  EthernetPhyTx &getTX() { return tx; }
};


EthernetPhy::
EthernetPhy(RunnableQueue &s, Port *TX_CLK, Port *RX_CLK, Port *RXD,
            Port *RX_DV, Port *RX_ER, Port *MDC, Port *MDIO,
            const std::string &ifname) :
  link(createNetworkLinkTap(ifname)),
  rx(s, RX_CLK, RXD, RX_DV, RX_ER),
  tx(s, TX_CLK),
  smi(s, MDC, MDIO)
{
  rx.setLink(link.get());
  tx.setLink(link.get());
}

static Peripheral *
createEthernetPhy(SystemState &system, const PortAliases &portAliases,
                  const Properties &properties)
{
  Port *TXD = properties.get("txd")->getAsPort().lookup(system, portAliases);
  Port *TX_EN =
    properties.get("tx_en")->getAsPort().lookup(system, portAliases);
  Port *TX_CLK =
    properties.get("tx_clk")->getAsPort().lookup(system, portAliases);
  Port *RX_CLK =
    properties.get("rx_clk")->getAsPort().lookup(system, portAliases);
  Port *RXD = properties.get("rxd")->getAsPort().lookup(system, portAliases);
  Port *RX_DV =
    properties.get("rx_dv")->getAsPort().lookup(system, portAliases);
  Port *RX_ER =
  properties.get("rx_er")->getAsPort().lookup(system, portAliases);
  Port *MDC =
    properties.get("mdc")->getAsPort().lookup(system, portAliases);
  Port *MDIO =
    properties.get("mdio")->getAsPort().lookup(system, portAliases);
  std::string ifname;
  if (const Property *property = properties.get("ifname"))
    ifname = property->getAsString();
  EthernetPhy *p =
    new EthernetPhy(system.getScheduler(), TX_CLK, RX_CLK, RXD, RX_DV, RX_ER,
                    MDC, MDIO, ifname);
  p->getTX().connectTXD(TXD);
  p->getTX().connectTX_EN(TX_EN);
  if (const Property *property = properties.get("tx_er"))
    p->getTX().connectTX_ER(property->getAsPort().lookup(system, portAliases));
  return p;
}

std::auto_ptr<PeripheralDescriptor> getPeripheralDescriptorEthernetPhy()
{
  std::auto_ptr<PeripheralDescriptor> p(
    new PeripheralDescriptor("ethernet-phy", &createEthernetPhy));
  p->addProperty(PropertyDescriptor::portProperty("txd")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("tx_en")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("tx_clk")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("tx_er"));
  p->addProperty(PropertyDescriptor::portProperty("rxd")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("rx_dv")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("rx_clk")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("rx_er")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("mdio"));
  p->addProperty(PropertyDescriptor::portProperty("mdc"));
  p->addProperty(PropertyDescriptor::stringProperty("ifname"));
  return p;
}
