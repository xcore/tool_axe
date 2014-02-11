// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Node.h"
#include "BitManip.h"

using namespace axe;

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
  type(t),
  jtagIndex(0),
  nodeID(0),
  parent(0),
  sswitch(this),
  nodeNumberBits(16)
{
  xLinks.resize(numXLinks);
}

Node::~Node()
{
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

void Node::setNodeNumberBits(unsigned value)
{
  nodeNumberBits = value;
  directions.resize(nodeNumberBits);
}

unsigned Node::getNodeNumberBits() const
{
  return nodeNumberBits;
}

unsigned Node::getNonNodeNumberBits() const
{
  return 16 - nodeNumberBits;
}

void Node::setNodeID(unsigned value)
{
  nodeID = value;
}

void Node::finalize()
{
  sswitch.initRegisters();
}

ChanEndpoint *Node::getIncomingChanendDest(ResourceID ID)
{
  Node *node = this;
  // Use Brent's algorithm to detect cycles.
  Node *tortoise = node;
  unsigned hops = 0;
  unsigned leapCount = 8;
  while (1) {
    unsigned destNode = ID.node() >> node->getNonNodeNumberBits();
    unsigned diff = destNode ^ node->getNodeID();
    if (diff == 0)
      break;
    // Lookup direction
    unsigned bit = 31 - countLeadingZeros(diff);
    unsigned direction = node->directions[bit];
    // Lookup Xlink.
    XLink *xLink = node->getXLinkForDirection(direction);
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
  return node->getLocalChanendDest(ID);
}

ChanEndpoint *Node::getOutgoingChanendDest(ResourceID ID)
{
  return getIncomingChanendDest(ID);
}

bool Node::hasMatchingNodeID(ResourceID ID)
{
  return (ID.node() >> getNonNodeNumberBits()) == nodeID;
}
