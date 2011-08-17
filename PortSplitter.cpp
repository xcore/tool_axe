// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PortSplitter.h"

void PortSplitter::seePinsChange(const Signal &value, ticks_t time)
{
  for (std::vector<PortInterface*>::iterator it = outputs.begin(),
       e = outputs.end(); it != e; ++it) {
    (*it)->seePinsChange(value, time);
  }
}
