// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PeripheralNode_h
#define _PeripheralNode_h

#include "Node.h"

namespace axe {

class PeripheralNode : public Node {
public:
  PeripheralNode();
  ChanEndpoint *getLocalChanendDest(ResourceID ID) override;
};

}  // End axe namespace

#endif // _PeripheralNode_h
