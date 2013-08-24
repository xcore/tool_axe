// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PeripheralNode.h"

using namespace axe;

PeripheralNode::PeripheralNode() : Node(Node::XS1_L, 1)
{
  setNodeNumberBits(0);
}

ChanEndpoint *PeripheralNode::getLocalChanendDest(ResourceID ID)
{
  return nullptr;
}
