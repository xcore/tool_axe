// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "BreakpointManager.h"
#include "Core.h"

using namespace axe;

bool BreakpointManager::
setBreakpoint(Core &c, uint32_t address, BreakpointType type)
{
  if (!c.setBreakpoint(address))
    return false;
  breakpointInfo[std::make_pair(&c, address)] = type;
  return true;
}

BreakpointType BreakpointManager::
getBreakpointType(Core &c, uint32_t address)
{
  auto entry = breakpointInfo.find(std::make_pair(&c, address));
  if (entry == breakpointInfo.end())
    return BreakpointType::Other;
  return entry->second;
}

void BreakpointManager::unsetBreakpoints()
{
  for (const auto &entry : breakpointInfo) {
    Core *core = entry.first.first;
    core->unsetBreakpoint(entry.first.second);
  }
  breakpointInfo.clear();
}
