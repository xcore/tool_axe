// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Node.h"
#include "Core.h"

Node::~Node()
{
  for (std::vector<Core*>::iterator it = cores.begin(), e = cores.end();
       it != e; ++it) {
    delete *it;
  }
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
  for (std::vector<Core*>::iterator it = cores.begin(), e = cores.end();
       it != e; ++it) {
    (*it)->seeNewNodeID();
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

unsigned Node::getCoreNumberBits() const
{
  unsigned bits = getMinimumBits(cores.size());
  if (getType() == Node::XS1_G && bits < 8)
    bits = 8;
  return bits;
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
