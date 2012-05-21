// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Options_h_
#define _Options_h_

#include "PortArg.h"
#include "Property.h"
#include <vector>
#include <string>

class PeripheralDescriptor;

typedef std::vector<std::pair<PortArg, PortArg> > LoopbackPorts;

struct Options {
  LoopbackPorts loopbackPorts;
  std::vector<std::pair<PeripheralDescriptor*, Properties> > peripherals;
  const char *file;
  std::string rom;
  std::string vcdFile;
  bool tracing;

  Options();
  void parse(int argc, char **argv);
};

#endif // _Options_h_
