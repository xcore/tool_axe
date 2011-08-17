// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PortNames_h_
#define _PortNames_h_

#include "Config.h"
#include <string>

bool getPortName(uint32_t id, std::string &name);
bool getPortId(const std::string &name, uint32_t &id);

#endif // _PortNames_h_
