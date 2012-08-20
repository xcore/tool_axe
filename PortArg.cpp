// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PortArg.h"
#include "PortNames.h"
#include <iostream>
#include <cstdlib>
#include "SystemState.h"
#include "Node.h"
#include "Core.h"
#include "PortAliases.h"

bool PortArg::parse(const std::string &s, PortArg &arg)
{
  size_t pos;
  std::string core;
  std::string port;
  if ((pos = s.find_first_of(':')) != std::string::npos) {
    port = s.substr(pos + 1);
    if (port.find_first_of(':') != std::string::npos)
      return false;
    core = s.substr(0, pos);
  } else {
    port = s;
  }
  arg = PortArg(core, port);
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
  for (SystemState::node_iterator outerIt = system.node_begin(),
       outerE = system.node_end(); outerIt != outerE; ++outerIt) {
    Node &node = **outerIt;
    for (Node::core_iterator innerIt = node.core_begin(),
         innerE = node.core_end(); innerIt != innerE; ++innerIt) {
      Core *core = *innerIt;
      if (s.empty() || core->getCodeReference() == s)
        return core;
    }
  }
  return 0;
}

static Port *
lookupAux(SystemState &system, const std::string &core, const std::string &port)
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

Port *PortArg::lookup(SystemState &system, const PortAliases &portAliases) const
{
  if (core.empty()) {
    std::string actualCore, actualPort;
    if (portAliases.lookup(port, actualCore, actualPort))
      return lookupAux(system, actualCore, actualPort);
  }
  return lookupAux(system, core, port);
}

void PortArg::dump(std::ostream &s) const
{
  s << port;
}
