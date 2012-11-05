// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _BreakpointManager_h
#define _BreakpointManager_h

#include <map>
#include <stdint.h>

namespace axe {

class Core;

enum class BreakpointType {
  Exception,
  Syscall,
  Other
};

class BreakpointManager {
  std::map<std::pair<Core*,uint32_t>,BreakpointType> breakpointInfo;
public:
  bool setBreakpoint(Core &c, uint32_t address, BreakpointType type);
  BreakpointType getBreakpointType(Core &c, uint32_t address);
  void unsetBreakpoints();
};

}

#endif // _BreakpointManager_h
