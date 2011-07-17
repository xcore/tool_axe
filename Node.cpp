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
