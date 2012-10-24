// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PortArg.h"
#include "PortNames.h"
#include <iostream>
#include <cstdlib>
#include <cerrno>
#include "SystemState.h"
#include "Node.h"
#include "Core.h"
#include "PortAliases.h"

static bool parsePortOffset(const std::string &s, signed char &result)
{
  char *endp;
  errno = 0;
  long value = std::strtol(s.c_str(), &endp, 10);
  if (errno != 0 || *endp != '\0')
    return false;
  if (value < 0 || value > 127)
    return false;
  result = value;
  return true;
}

bool PortArg::parse(const std::string &s, PortArg &arg)
{
  size_t pos;
  if (s.empty())
    return false;
  
  std::string index;
  std::string port;
  if (s[s.length() - 1] == ']') {
    size_t closePos = s.length() - 1;
    size_t openPos = s.find_last_of('[', closePos - 1);
    if (openPos == std::string::npos)
      return false;
    port = s.substr(0, openPos);
    index = s.substr(openPos + 1, closePos - (openPos + 1));
    if (index.empty())
      return false;
  } else {
    port = s;
  }

  signed char lowerBound = 0;
  signed char upperBound = -1;
  if (!index.empty()) {
    std::string lowerBoundStr, upperBoundStr;
    if ((pos = index.find_first_of(':')) != std::string::npos) {
      lowerBoundStr = index.substr(0, pos);
      upperBoundStr = index.substr(pos + 1);
      if (lowerBoundStr.empty() || upperBoundStr.empty())
        return false;
    } else {
      lowerBoundStr = index;
    }
    if (!parsePortOffset(lowerBoundStr, lowerBound))
      return false;
    if (upperBoundStr.empty()) {
      upperBound = lowerBound + 1;
    } else {
      if (!parsePortOffset(upperBoundStr, upperBound))
        return false;
      if (upperBound <= lowerBound)
        return false;
    }
  }

  std::string core;
  std::string identifier;
  if ((pos = port.find_first_of(':')) != std::string::npos) {
    identifier = port.substr(pos + 1);
    core = port.substr(0, pos);
    if (core.empty())
      return false;
  } else {
    identifier = port;
  }
  arg = PortArg(core, identifier, lowerBound, upperBound);
  return true;
};

static bool convertPortString(const std::string &s, uint32_t &id)
{
  if (s.substr(0, 4) == "XS1_") {
    return getPortId(s.substr(4), id);
  }
  if (getPortId(s, id)) {
    return true;
  }
  char *endp;
  long value = std::strtol(s.c_str(), &endp, 0);
  if (*endp != '\0')
    return false;
  id = value;
  return true;
}

/// Finds the first core match with the specified code reference. If the
/// code reference is empty then the first core is returned.
static Core *findMatchingCore(const std::string &s, SystemState &system)
{
  for (auto outerIt = system.node_begin(), outerE = system.node_end();
       outerIt != outerE; ++outerIt) {
    Node &node = **outerIt;
    for (auto innerIt = node.core_begin(), innerE = node.core_end();
         innerIt != innerE; ++innerIt) {
      Core *core = *innerIt;
      if (s.empty() || core->getCodeReference() == s)
        return core;
    }
  }
  return 0;
}

static Port *
lookupPortAux(SystemState &system, const std::string &core,
              const std::string &port)
{
  Core *c = findMatchingCore(core, system);
  if (!c)
    return 0;
  uint32_t id;
  if (!convertPortString(port, id))
    return 0;
  Resource *res = c->getResourceByID(id);
  if (!res || res->getType() != RES_TYPE_PORT)
    return 0;
  return static_cast<Port*>(res);
}

Port *PortArg::
lookupPort(SystemState &system, const PortAliases &portAliases) const
{
  if (core.empty()) {
    std::string actualCore, actualPort;
    if (portAliases.lookup(port, actualCore, actualPort))
      return lookupPortAux(system, actualCore, actualPort);
  }
  return lookupPortAux(system, core, port);
}

bool PortArg::
lookup(SystemState &system, const PortAliases &portAliases, Port *&portResult,
       unsigned &beginOffsetResult, unsigned &endOffsetResult) const
{
  Port *p = lookupPort(system, portAliases);
  if (!p)
    return false;
  signed char adjustedEndOffset = endOffset;
  if (adjustedEndOffset == -1)
    adjustedEndOffset = p->getPortWidth();
  else if ((unsigned)adjustedEndOffset > p->getPortWidth())
    return false;
  if (beginOffset >= adjustedEndOffset)
    return false;
  portResult = p;
  beginOffsetResult = beginOffset;
  endOffsetResult = adjustedEndOffset;
  return true;
}

void PortArg::dump(std::ostream &s) const
{
  if (!core.empty())
    s << core << ':';
  s << port;
  if (beginOffset != 0 || endOffset != -1) {
    s << '[' << (unsigned)beginOffset;
    if (endOffset != beginOffset + 1) {
      s << ':' << (unsigned)endOffset;
    }
    s << ']';
  }
}
