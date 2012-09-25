// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PortConnectionManager.h"
#include "PortArg.h"
#include "Port.h"
#include "Property.h"

void PortConnectionWrapper::attach(PortInterface *p) {
  port->setLoopback(p);
}

PortInterface *PortConnectionWrapper::getInterface() {
  return port;
}

PortConnectionManager::
PortConnectionManager(SystemState &sys, PortAliases &aliases) :
  system(sys), portAliases(aliases)
{
}

PortConnectionWrapper PortConnectionManager::get(const PortArg &arg)
{
  return PortConnectionWrapper(arg.lookup(system, portAliases));
}

PortConnectionWrapper PortConnectionManager::
get(const Properties &properties, const std::string &name)
{
  const Property *property = properties.get(name);
  if (!property)
    return PortConnectionWrapper(0);
  return get(property->getAsPort());
}
