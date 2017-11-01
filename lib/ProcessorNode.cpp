// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include <iostream>
#include "ProcessorNode.h"
#include "Core.h"

using namespace axe;

ProcessorNode::ProcessorNode(Type t, unsigned numXLinks) :
  Node(t, numXLinks)
{
  // std::cout << "Creating ProcessorNode\n";
}

ProcessorNode::~ProcessorNode()
{
  for (Core *core : cores) {
    delete core;
  }
}

void ProcessorNode::setNodeID(unsigned value)
{
  Node::setNodeID(value);
  for (Core *core : cores) {
    core->updateIDs();
  }
}

void ProcessorNode::finalize()
{
  computeCoreNumberBits();
  if (type == XS1_G) {
    for (unsigned i = 0, e = getNodeNumberBits(); i != e; ++i) {
      setDirection(i, i);
    }
  }
  for (Core *core : cores) {
    core->updateIDs();
    core->finalize();
  }
  Node::finalize();
}

void ProcessorNode::addCore(std::unique_ptr<Core> c)
{
  c->setParent(this);
  cores.push_back(c.get());
  c.release();
}

/// Returns the minimum number of bits needed to encode the specified number
/// of values.
static unsigned getMinimumBits(unsigned values)
{
  if (values == 0)
    return 0;
  return 32 - countLeadingZeros(values - 1);
}

void ProcessorNode::computeCoreNumberBits()
{
  unsigned coreNumberBits = getMinimumBits(cores.size());
  if (getType() == XS1_G && coreNumberBits < 8)
    coreNumberBits = 8;
  setNodeNumberBits(16 - coreNumberBits);
}

uint32_t ProcessorNode::getCoreID(unsigned coreNum) const
{
  //unsigned coreBits = getNonNodeNumberBits();
  assert(coreNum <= makeMask(getNonNodeNumberBits()));
  //auto id = (getNodeID() << coreBits) | coreNum;
  auto id = getNodeID() | coreNum;
  std::cout << "Returning processor node 0x" << std::hex << id << std::endl;
  return id;
}

bool ProcessorNode::getTypeFromJtagID(unsigned jtagID, Type &type)
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
  case 0x5633:
    type = XS2_A;
    return true;
  }
}

ChanEndpoint *ProcessorNode::getLocalChanendDest(ResourceID ID)
{
  //assert(hasMatchingNodeID(ID));
  unsigned destCore = ID.node() & makeMask(getNonNodeNumberBits());
  if (destCore >= cores.size())
    return 0;
  ChanEndpoint *dest = 0;
  cores[destCore]->getLocalChanendDest(ID, dest);
  return dest;
}
