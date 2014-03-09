// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PortCombiner.h"
#include "Signal.h"
#include "BitManip.h"

using namespace axe;

PortCombiner::PortCombiner(RunnableQueue &scheduler) :
  PortHandleClockMixin<PortCombiner>(scheduler),
  value(0) {}

void PortCombiner::seePinsValueChange(uint32_t newValue, ticks_t time)
{
  for (const Slice &slice : slices) {
    uint32_t mask = slice.mask;
    unsigned shift = slice.shift;
    PortInterface *next = slice.interface;
    uint32_t oldSliceValue = (value & mask) >> shift;
    uint32_t newSliceValue = (newValue & mask) >> shift;
    if (newSliceValue != oldSliceValue) {
      next->seePinsChange(Signal(newSliceValue), time);
    }
  }
  value = newValue;
}

void PortCombiner::attach(PortInterface *to, unsigned beginOffset,
                          unsigned endOffset)
{
  unsigned shift = beginOffset;
  uint32_t mask = makeMask(endOffset - beginOffset) << shift;
  slices.push_back(Slice(mask, shift, to));
}
