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
#include <memory>

// MII uses a 25MHz clock.
const uint32_t ethernetPhyHalfPeriod = CYCLES_PER_TICK * 2;
const unsigned interframeGap = (12 * 8) / 4; // Time to transmit 12 bytes.
const uint32_t CRC32Poly = 0xEDB88320;

class EthernetPhyTx {
  void seeTXDChange(const Signal &value, ticks_t time);
  void seeTX_ENChange(const Signal &value, ticks_t time);
  void seeTX_ERChange(const Signal &value, ticks_t time);

  NetworkLink *link;
  Port *TX_CLK;
  PortInterfaceMemberFuncDelegate<EthernetPhyTx> TXDProxy;
  PortInterfaceMemberFuncDelegate<EthernetPhyTx> TX_ENProxy;
  PortInterfaceMemberFuncDelegate<EthernetPhyTx> TX_ERProxy;
  unsigned TX_CLKValue;
  Signal TXDSignal;
  Signal TX_ENSignal;
  Signal TX_ERSignal;

  bool inFrame;
  bool hadError;
  std::vector<uint8_t> frame;
  bool hasPrevNibble;
  uint8_t prevNibble;

  void reset();
public:
  EthernetPhyTx(RunnableQueue &sched, Port *MISO);
  void setLink(NetworkLink *value) { link = value; }
  void connectTXD(Port *p) { p->setLoopback(&TXDProxy); }
  void connectTX_EN(Port *p) { p->setLoopback(&TX_ENProxy); }
  void connectTX_ER(Port *p) { p->setLoopback(&TX_ERProxy); }
  virtual void clock(ticks_t time);
};

EthernetPhyTx::EthernetPhyTx(RunnableQueue &s, Port *txclk) :
  link(0),
  TX_CLK(txclk),
  TXDProxy(*this, &EthernetPhyTx::seeTXDChange),
  TX_ENProxy(*this, &EthernetPhyTx::seeTX_ENChange),
  TX_ERProxy(*this, &EthernetPhyTx::seeTX_ERChange),
  TX_CLKValue(0),
  TXDSignal(0),
  TX_ENSignal(0),
  TX_ERSignal(0)
{
  reset();
  TX_CLK->seePinsChange(Signal(0, CYCLES_PER_TICK * 2), 0);
}

void EthernetPhyTx::reset()
{
  inFrame = false;
  hadError = false;
  hasPrevNibble = false;
  frame.clear();
}

#include <iostream>

void EthernetPhyTx::seeTXDChange(const Signal &value, ticks_t time)
{
  TXDSignal = value;
}

void EthernetPhyTx::seeTX_ENChange(const Signal &value, ticks_t time)
{
  TX_ENSignal = value;
}

void EthernetPhyTx::seeTX_ERChange(const Signal &value, ticks_t time)
{
  TX_ERSignal = value;
}

void EthernetPhyTx::clock(ticks_t time)
{
  TX_CLKValue = !TX_CLKValue;
  TX_CLK->seePinsChange(Signal(TX_CLKValue), time);
  if (TX_CLKValue) {
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
        if (!hadError && !frame.empty()) {
          link->transmitFrame(&frame[0], frame.size());
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
  }
}

class EthernetPhyRx {
  NetworkLink *link;
  Port *RX_CLK;
  Port *RXD;
  Port *RX_DV;
  Port *RX_ER;
  unsigned RX_CLKValue;
  uint8_t frame[NetworkLink::maxFrameSize];
  unsigned frameSize;
  unsigned nibblesReceieved;
  unsigned interFrameNibblesRemaining;
  enum State {
    IDLE,
    TX_SFD2,
    TX_FRAME,
    TX_EFD,
    INTER_FRAME,
  } state;
  void appendCRC32();
  bool receiveFrame();
public:
  EthernetPhyRx(Port *rxclk, Port *rxd, Port *rxdv, Port *rxer);

  void setLink(NetworkLink *value) { link = value; }
  void clock(ticks_t time);
};

EthernetPhyRx::EthernetPhyRx(Port *rxclk, Port *rxd, Port *rxdv, Port *rxer) :
  link(0),
  RX_CLK(rxclk),
  RXD(rxd),
  RX_DV(rxdv),
  RX_ER(rxer),
  RX_CLKValue(0),
  state(IDLE)
{
}

void EthernetPhyRx::appendCRC32()
{
  uint32_t crc = 0;
  for (unsigned i = 0; i < frameSize; i++) {
    uint8_t data = frame[i];
    if (i < 4)
      data = ~data;
    crc = crc8(crc, data, CRC32Poly);
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
  const unsigned minSize = 64 - 4;
  if (frameSize < minSize) {
    std::memset(&frame[frameSize], 0, minSize - frameSize);
    frameSize = minSize;
  }
  appendCRC32();
  return true;
}

void EthernetPhyRx::clock(ticks_t time)
{
  RX_CLKValue = !RX_CLKValue;
  RX_CLK->seePinsChange(Signal(RX_CLKValue), time);
  if (RX_CLKValue == 0) {
    // Drive on falling edge.
    switch (state) {
    case IDLE:
      if (receiveFrame()) {
        RXD->seePinsChange(Signal(0x5), time);
        RX_DV->seePinsChange(Signal(1), time);
        state = TX_SFD2;
      }
      break;
    case TX_SFD2:
      RXD->seePinsChange(Signal(0xd), time);
      nibblesReceieved = 0;
      state = TX_FRAME;
      break;
    case TX_FRAME:
      {
        unsigned byteNum = nibblesReceieved / 2;
        unsigned nibbleNum = nibblesReceieved % 2;
        unsigned data = (frame[byteNum] >> (nibbleNum * 4)) & 0xff;
        RXD->seePinsChange(Signal(data), time);
        ++nibblesReceieved;
        if (nibblesReceieved == frameSize * 2) {
          state = TX_EFD;
        }
      }
      break;
    case TX_EFD:
      RXD->seePinsChange(Signal(0), time);
      RX_DV->seePinsChange(Signal(0), time);
      state = INTER_FRAME;
      interFrameNibblesRemaining = interframeGap;
      break;
    case INTER_FRAME:
      if (--interFrameNibblesRemaining == 0) {
        state = IDLE;
      }
      break;
    }
  }
}

class EthernetPhy : public Peripheral, public Runnable {
  std::auto_ptr<NetworkLink> link;
  EthernetPhyRx rx;
  EthernetPhyTx tx;
  RunnableQueue &scheduler;
public:
  EthernetPhy(RunnableQueue &s, Port *TX_CLK, Port *RX_CLK, Port *RXD,
              Port *RX_DV, Port *RX_ER);
  EthernetPhyTx &getTX() { return tx; }
  void run(ticks_t time);
};


EthernetPhy::
EthernetPhy(RunnableQueue &s, Port *TX_CLK, Port *RX_CLK, Port *RXD,
            Port *RX_DV, Port *RX_ER) :
  link(createNetworkLinkTap()),
  rx(RX_CLK, RXD, RX_DV, RX_ER),
  tx(s, TX_CLK),
  scheduler(s)
{
  rx.setLink(link.get());
  tx.setLink(link.get());
  scheduler.push(*this, ethernetPhyHalfPeriod);
}

void EthernetPhy::run(ticks_t time)
{
  rx.clock(time);
  tx.clock(time);
  scheduler.push(*this, time + ethernetPhyHalfPeriod);
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
  EthernetPhy *p =
    new EthernetPhy(system.getScheduler(), TX_CLK, RX_CLK, RXD, RX_DV, RX_ER);
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
  return p;
}
