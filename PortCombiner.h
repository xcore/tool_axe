// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PortCombiner_
#define _PortCombiner_

#include <map>
#include "PortHandleClockMixin.h"

class PortCombiner : public PortHandleClockMixin<PortCombiner> {
  uint32_t value;
  std::map<std::pair<uint32_t,unsigned>,PortInterface*> slices;
public:
  PortCombiner(RunnableQueue &s);
  void seePinsValueChange(uint32_t value, ticks_t time);
  void attach(PortInterface *to, unsigned beginOffset, unsigned endOffset);
};

#endif //_PortCombiner_
