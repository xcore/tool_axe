// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Node.h"
#include "Core.h"

XLink::XLink() :
  destNode(0),
  destXLinkNum(0),
  enabled(false),
  fiveWire(false),
  network(0),
  direction(0),
  // TODO find out defaults.
  interTokenDelay(0),
  interSymbolDelay(0)
{
  
}

const XLink *XLink::getDestXLink() const
{
  if (!destNode)
    return 0;
  return &destNode->getXLink(destXLinkNum);
}

bool XLink::isConnected() const
{
  if (!isEnabled())
    return false;
  const XLink *otherEnd = getDestXLink();
  if (!otherEnd || !otherEnd->isEnabled())
    return false;
  return isFiveWire() == otherEnd->isFiveWire();
}

Node::Node(Type t, unsigned numXLinks) :
  jtagIndex(0),
  nodeID(0),
  parent(0),
  type(t),
  sswitch(this),
  coreNumberBits(0)
{
  xLinks.resize(numXLinks);
}

Node::~Node()
{
  for (Core *core : cores) {
    delete core;
  }
}

void Node::finalize()
{
  computeCoreNumberBits();
  directions.resize(getNodeNumberBits());
  if (type == XS1_G) {
    for (unsigned i = 0, e = directions.size(); i != e; ++i) {
      directions[i] = i;
    }
  }
  for (Core *core : cores) {
    core->updateIDs();
    core->finalize();
  }
  sswitch.initRegisters();
}

void Node::addCore(std::auto_ptr<Core> c)
{
  c->setParent(this);
  cores.push_back(c.get());
  c.release();
}


void Node::setNodeID(unsigned value)
{
  nodeID = value;
  for (Core *core : cores) {
    core->updateIDs();
  }
}

/// Returns the minimum number of bits needed to encode the specified number
/// of values.
static unsigned getMinimumBits(unsigned values)
{
  if (values == 0)
    return 0;
  return 32 - countLeadingZeros(values - 1);
}

void Node::computeCoreNumberBits()
{
  coreNumberBits = getMinimumBits(cores.size());
  if (getType() == Node::XS1_G && coreNumberBits < 8)
    coreNumberBits = 8;
}


unsigned Node::getCoreNumberBits() const
{
  return coreNumberBits;
}

unsigned Node::getNodeNumberBits() const
{
  return 16 - coreNumberBits;
}

uint32_t Node::getCoreID(unsigned coreNum) const
{
  unsigned coreBits = getCoreNumberBits();
  assert(coreNum <= makeMask(getCoreNumberBits()));
  return (getNodeID() << coreBits) | coreNum;
}

bool Node::getTypeFromJtagID(unsigned jtagID, Type &type)
{
  switch (jtagID) {
  default:
    return false;
  case 0x2731:
    type = XS1_G;
    return true;
  case 0x2633:
    type = XS1_L;
    return true;
  }
}

bool Node::hasMatchingNodeID(ResourceID ID)
{
  switch (type) {
    default: assert(0 && "Unexpected node type");
    case Node::XS1_G:
      return ((ID.node() >> 8) & 0xff) == nodeID;
    case Node::XS1_L:
      return ID.node() == nodeID;
  }
}

void Node::connectXLink(unsigned num, Node *destNode, unsigned destNum)
{
  xLinks[num].destNode = destNode;
  xLinks[num].destXLinkNum = destNum;
}

XLink *Node::getXLinkForDirection(unsigned direction)
{
  for (XLink &xLink : xLinks) {
    if (xLink.isEnabled() && xLink.getDirection() == direction)
      return &xLink;
  }
  return 0;
}

ChanEndpoint *Node::getChanendDest(ResourceID ID)
{
  Node *node = this;
  // Use Brent's algorithm to detect cycles.
  Node *tortoise = node;
  unsigned hops = 0;
  unsigned leapCount = 8;
  while (1) {
    unsigned destNode = ID.node() >> node->getCoreNumberBits();
    unsigned diff = destNode ^ node->getNodeID();
    if (diff == 0)
      break;
    // Lookup direction
    unsigned bit = countLeadingZeros(diff) + getNodeNumberBits() - 32;
    unsigned direction = directions[bit];
    // Lookup Xlink.
    XLink *xLink = getXLinkForDirection(direction);
    if (!xLink || !xLink->isConnected())
      return 0;
    node = xLink->destNode;
    ++hops;
    // Junk message if a cycle is detected.
    if (node == tortoise)
      return 0;
    if (hops == leapCount) {
      leapCount <<= 1;
      tortoise = node;
    }
  }
  if (ID.isConfig() && ID.num() == RES_CONFIG_SSCTRL) {
    return &node->sswitch;
  }
  unsigned destCore = ID.node() & makeMask(node->getCoreNumberBits());
  if (destCore >= node->cores.size())
    return 0;
  ChanEndpoint *dest = 0;
  node->getCores()[destCore]->getLocalChanendDest(ID, dest);
  return dest;
}
