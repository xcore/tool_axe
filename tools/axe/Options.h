// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Options_h_
#define _Options_h_

#include "Config.h"
#include "PortArg.h"
#include "Property.h"
#include <vector>
#include <string>

namespace axe {

class PeripheralDescriptor;

typedef std::vector<std::pair<PortArg, PortArg> > LoopbackPorts;

struct Options {
  enum BootMode {
    BOOT_SIM,
    BOOT_SPI
  };
  BootMode bootMode;
  LoopbackPorts loopbackPorts;
  std::vector<std::pair<PeripheralDescriptor*, Properties> > peripherals;
  const char *file;
  std::string rom;
  std::string vcdFile;
  bool tracing;
  ticks_t maxCycles;

  Options();
  void parse(int argc, char **argv);
};
  
} // End axe namespace

#endif // _Options_h_
