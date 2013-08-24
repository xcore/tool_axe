// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "SSwitchCtrlRegs.h"
#include "ProcessorNode.h"
#include "BitManip.h"
#include <algorithm>

using namespace axe;

namespace SSwitchReg {
  enum {
    DEVICE_ID0 = 0x0,
    DEVICE_ID1 = 0x1,
    DEVICE_ID2 = 0x2,
    DEVICE_ID3 = 0x3,
    NODE_CONFIG = 0x4,
    NODE_ID = 0x5,
    PLL_CTL = 0x6,
    CLK_DIVIDER = 0x7,
    REF_CLK_DIVIDER = 0x8,
    DIMENSION_DIRECTION_0 = 0xc,
    DIMENSION_DIRECTION_1 = 0xd,
    XCORE0_GLOBAL_DEBUG_CONFIG = 0x10,
    GLOBAL_DEBUG_SOURCE = 0x1f,
    SLINK_0 = 0x20, // 8 slinks
    PLINK_0 = 0x40, // 4 plinks
    XLINK_0 = 0x80, // 8 xlinks
    XSTATIC_0 = 0xa0, // 8
  };
};

SSwitchCtrlRegs::SSwitchCtrlRegs(Node *n) :
  node(n),
  scratchReg(0)
{
}

void SSwitchCtrlRegs::initReg(unsigned num, uint8_t flags)
{
  regFlags[num] = flags;
}

void SSwitchCtrlRegs::initRegisters()
{
  using namespace SSwitchReg;
  // There is a limit to how many links can be addressed via registers.
  unsigned numXLinks = std::min(node->getNumXLinks(),
                                (unsigned)PLINK_0 - SLINK_0);
  unsigned numRegs = XSTATIC_0 + node->getNumXLinks();
  regFlags.resize(numRegs);
  
  initReg(NODE_ID, REG_RW);
  if (node->getType() == Node::XS1_G) {
    initReg(DEVICE_ID3, REG_RW);
  }
  unsigned numDirectionRegisters = (node->getNodeNumberBits() + 7) / 8;
  if (node->getType() != Node::XS1_G) {
    for (unsigned i = 0; i < numDirectionRegisters; i++) {
      initReg(DIMENSION_DIRECTION_0 + i, REG_RW);
    }
  }
  for (unsigned i = 0; i < numXLinks; i++) {
    initReg(SLINK_0 + i, REG_RW);
    initReg(XLINK_0 + i, REG_RW);
    //initReg(XSTATIC_0 + i, REG_RW);
  }
}

static inline void
setBitRange(uint32_t &x, uint32_t value, unsigned high, unsigned low)
{
  assert(high >= low && high < 32);
  unsigned shift = low;
  unsigned mask = makeMask(high - low + 1);
  x &= ~mask;
  x |= (value & mask) << shift;
}

static inline void setBit(uint32_t &x, uint32_t value, unsigned bit)
{
  return setBitRange(x, value, bit, bit);
}

static inline uint32_t getBits(uint32_t value, unsigned shift, unsigned size)
{
  return (value >> shift) & ((1 << size) - 1);
}

static inline uint32_t getBitRange(uint32_t value, unsigned high, unsigned low)
{
  return getBits(value, low, 1 + high - low);
}

static inline uint32_t getBit(uint32_t value, unsigned shift)
{
  return getBits(value, shift, 1);
}

static uint32_t readXLinkStateReg(const Node *node, const XLink &xLink)
{
  unsigned value = 0;
  if (node->getType() == Node::XS1_G) {
    setBitRange(value, xLink.getInterTokenDelay(), 3, 0);
    setBitRange(value, xLink.getInterSymbolDelay(), 11, 8);
  } else {
    setBitRange(value, xLink.getInterTokenDelay(), 10, 0);
    setBitRange(value, xLink.getInterSymbolDelay(), 21, 11);
  }
  bool isConnected = xLink.isConnected();
  if (node->getType() != Node::XS1_G) {
    setBit(value, isConnected, 25);
    setBit(value, isConnected, 26);
  }
  setBit(value, xLink.isFiveWire(), 30);
  setBit(value, xLink.isEnabled(), 31);
  return value;
}

static void writeXLinkStateReg(const Node *node, XLink &xLink, uint32_t value)
{
  if (node->getType() == Node::XS1_G) {
    xLink.setInterTokenDelay(getBitRange(value, 3, 0));
    xLink.setInterSymbolDelay(getBitRange(value, 11, 8));
  } else {
    xLink.setInterTokenDelay(getBitRange(value, 10, 0));
    xLink.setInterSymbolDelay(getBitRange(value, 21, 11));
  }
  // Ignore RESET and HELLO.
  xLink.setFiveWire(getBit(value, 30));
  xLink.setEnabled(getBit(value, 31));
}

static unsigned getDirectionBits(const Node *node)
{
  if (node->getNodeNumberBits() == 0)
    return 0;
  return 32 - countLeadingZeros(node->getNodeNumberBits() - 1);
}

static uint32_t
readXLinkDirectionAndNetworkReg(const Node *node, const XLink &xLink)
{
  unsigned value = 0;
  unsigned directionBits = getDirectionBits(node);
  setBitRange(value, xLink.getNetwork(), 5, 4);
  setBitRange(value, xLink.getDirection(), 8 + directionBits + 1, 8);
  return value;
}

static void
writeXLinkDirectionAndNetworkReg(const Node *node, XLink &xLink, uint32_t value)
{
  unsigned directionBits = getDirectionBits(node);
  xLink.setNetwork(getBitRange(value, 5, 4));
  xLink.setDirection(getBitRange(value, 8 + directionBits + 1, 8));
}

static uint32_t readDirectionReg(const Node *node, unsigned offset)
{
  unsigned value = 0;
  unsigned end = std::min(offset + 4, node->getNodeNumberBits());
  for (unsigned i = offset; i < end; ++i) {
    value |= node->getDirection(i) << ((i - offset) * 4);
  }
  return value;
}

static void writeDirectionReg(Node *node, unsigned offset, uint32_t value)
{
  unsigned end = std::min(offset + 4, node->getNodeNumberBits());
  for (unsigned i = offset; i < end; ++i) {
    unsigned direction = (value >> ((i - offset) * 4)) & 0xf;
    node->setDirection(i, direction);
  }
}

bool SSwitchCtrlRegs::read(uint16_t num, uint32_t &result)
{
  using namespace SSwitchReg;
  if (num > regFlags.size())
    return false;
  if ((regFlags[num] & REG_READ) == 0)
    return false;
  if (num >= SLINK_0 && num < PLINK_0) {
    result = readXLinkDirectionAndNetworkReg(node,
                                             node->getXLink(num - SLINK_0));
  }
  if (num >= XLINK_0 && num < XSTATIC_0) {
    result = readXLinkStateReg(node, node->getXLink(num - XLINK_0));
    return true;
  }
  switch (num) {
  case DIMENSION_DIRECTION_0:
  case DIMENSION_DIRECTION_1:
    result = readDirectionReg(node, (num - DIMENSION_DIRECTION_0) * 4);
    return true;
  case NODE_ID:
    result = node->getNodeID();
    return true;
  case DEVICE_ID3:
    result = scratchReg;
    return true;
  }
  assert(0 && "Unexpected register");
  return false;
}

bool SSwitchCtrlRegs::write(uint16_t num, uint32_t value)
{
  using namespace SSwitchReg;
  if (num > regFlags.size())
    return false;
  if ((regFlags[num] & REG_WRITE) == 0)
    return false;
  if (num >= SLINK_0 && num < PLINK_0) {
    writeXLinkDirectionAndNetworkReg(node,
                                     node->getXLink(num - SLINK_0), value);
    return true;
  }
  if (num >= XLINK_0 && num < XSTATIC_0) {
    writeXLinkStateReg(node, node->getXLink(num - XLINK_0), value);
    return true;
  }
  switch (num) {
  case DIMENSION_DIRECTION_0:
  case DIMENSION_DIRECTION_1:
    writeDirectionReg(node, (num - DIMENSION_DIRECTION_0) * 4, value);
    return true;
  case NODE_ID:
    node->setNodeID(value & makeMask(node->getNodeNumberBits()));
    return true;
  case DEVICE_ID3:
    scratchReg = value;
    return true;
  }
  assert(0 && "Unexpected register");
  return false;
}
