// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PortCombiner_
#define _PortCombiner_

#include <vector>
#include "PortHandleClockMixin.h"

namespace axe {

class PortCombiner final : public PortHandleClockMixin<PortCombiner> {
  uint32_t value;
  struct Slice {
    Slice(uint32_t mask, unsigned shift, PortInterface *interface) :
      mask(mask), shift(shift), interface(interface) {}
    uint32_t mask;
    unsigned shift;
    PortInterface *interface;
  };
  std::vector<Slice> slices;
public:
  PortCombiner(RunnableQueue &s);
  void seePinsValueChange(uint32_t value, ticks_t time);
  void attach(PortInterface *to, unsigned beginOffset, unsigned endOffset);
};
  
} // End axe namespace

#endif //_PortCombiner_
