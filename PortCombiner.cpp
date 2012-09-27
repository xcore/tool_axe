// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PortCombiner.h"
#include "Signal.h"
#include "BitManip.h"

PortCombiner::PortCombiner() : value(0) {}

void PortCombiner::seePinsChange(const Signal &signal, ticks_t time)
{
  assert(!signal.isClock());
  uint32_t newValue = signal.getValue(time);
  for (std::map<std::pair<uint32_t,unsigned>,PortInterface*>::iterator
       it = slices.begin(), e = slices.end(); it != e; ++it) {
    uint32_t mask = it->first.first;
    unsigned shift = it->first.second;
    PortInterface *next = it->second;
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
  slices.insert(std::make_pair(std::make_pair(mask, shift), to));
}
