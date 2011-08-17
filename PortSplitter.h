// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PortSplitter_h_
#define _PortSplitter_h_

#include "PortInterface.h"
#include <vector>

class PortSplitter : public PortInterface {
  std::vector<PortInterface *>outputs;
public:
  void add(PortInterface *pi) {
    outputs.push_back(pi);
  }
  bool empty() const { return outputs.empty(); }
  virtual void seePinsChange(const Signal &value, ticks_t time);
};

#endif // _PortSplitter_h_
