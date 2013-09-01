// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PortSplitter.h"
#include "PortHandleClockMixin.h"
#include "BitManip.h"
#include <cassert>

using namespace axe;

class axe::PortSplitterSlice final : public PortHandleClockMixin<PortSplitterSlice> {
  PortSplitter *parent;
  unsigned shift;
  uint32_t mask;
public:
  PortSplitterSlice(PortSplitter *p, RunnableQueue &scheduler, unsigned s,
                    uint32_t m) :
    PortHandleClockMixin<PortSplitterSlice>(scheduler),
    parent(p),
    shift(s),
    mask(m) {}
  void seePinsValueChange(uint32_t value, ticks_t time);
};

void PortSplitterSlice::seePinsValueChange(uint32_t value, ticks_t time)
{
  parent->seePinsValueChange(value, time, shift, mask);
}

PortSplitter::PortSplitter(RunnableQueue &s, PortInterface *p) :
  scheduler(s),
  port(p),
  value(0)
{
}

PortSplitter::~PortSplitter()
{
  for (auto &entry : slices) {
    delete entry.second;
  }
}

PortInterface *PortSplitter::
getInterface(unsigned beginOffset, unsigned endOffset)
{
  PortSplitterSlice *&slice = slices[std::make_pair(beginOffset, endOffset)];
  if (!slice) {
    unsigned shift = beginOffset;
    uint32_t mask = makeMask(endOffset - beginOffset) << beginOffset;
    slice = new PortSplitterSlice(this, scheduler, shift, mask);
  }
  return slice;
}

void PortSplitter::
seePinsValueChange(uint32_t sliceValue, ticks_t time, unsigned shift,
                   uint32_t mask)
{
  uint32_t newValue = value;
  newValue ^= ((sliceValue << shift) ^ newValue) & mask;
  if (newValue != value) {
    value = newValue;
    port->seePinsChange(Signal(value), time);
  }
}
