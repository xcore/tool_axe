// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PortSplitter_
#define _PortSplitter_

#include <map>
#include "Signal.h"

class PortInterface;
class PortSplitterSlice;

class PortSplitter {
  PortInterface *port;
  uint32_t value;
  std::map<std::pair<unsigned,unsigned>,PortSplitterSlice*> slices;
public:
  PortSplitter(PortInterface *p);
  ~PortSplitter();
  PortInterface *getInterface(unsigned beginOffset, unsigned endOffset);
  void seePinsChange(const Signal &signal, ticks_t time, unsigned shift,
                     uint32_t mask);
};

#endif // _PortSplitter_
