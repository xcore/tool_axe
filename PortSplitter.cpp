// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PortSplitter.h"
#include "PortInterface.h"
#include "BitManip.h"
#include <cassert>

class PortSplitterSlice : public PortInterface {
  PortSplitter *parent;
  unsigned shift;
  uint32_t mask;
public:
  PortSplitterSlice(PortSplitter *p, unsigned s, uint32_t m) :
    parent(p), shift(s), mask(m) {}
  virtual ~PortSplitterSlice();
  void seePinsChange(const Signal &value, ticks_t time);
};

PortSplitterSlice::~PortSplitterSlice()
{
}

void PortSplitterSlice::seePinsChange(const Signal &signal, ticks_t time)
{
  parent->seePinsChange(signal, time, shift, mask);
}

PortSplitter::PortSplitter(PortInterface *p) :
  port(p),
  value(0)
{
}

PortSplitter::~PortSplitter()
{
  for (std::map<std::pair<unsigned,unsigned>,PortSplitterSlice*>::iterator
       it = slices.begin(), e = slices.end(); it != e; ++it) {
    delete it->second;
  }
}

PortInterface *PortSplitter::
getInterface(unsigned beginOffset, unsigned endOffset)
{
  PortSplitterSlice *&slice = slices[std::make_pair(beginOffset, endOffset)];
  if (!slice) {
    unsigned shift = beginOffset;
    uint32_t mask = makeMask(endOffset - beginOffset) << beginOffset;
    slice = new PortSplitterSlice(this, shift, mask);
  }
  return slice;
}

void PortSplitter::seePinsChange(const Signal &signal, ticks_t time, unsigned shift,
                                 uint32_t mask)
{
  assert(!signal.isClock());
  uint32_t newValue = value;
  newValue ^= ((signal.getValue(time) << shift) ^ newValue) & mask;
  if (newValue != value) {
    value = newValue;
    port->seePinsChange(Signal(value), time);
  }
}
