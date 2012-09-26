// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PortConnectionManager.h"
#include "PortArg.h"
#include "Port.h"
#include "Property.h"

bool PortConnectionWrapper::isEntirePort() const
{
  if (!port)
    return true;
  return beginOffset == 0 && endOffset == port->getPortWidth();
}

void PortConnectionWrapper::attach(PortInterface *p) {
  if (isEntirePort())
    port->setLoopback(p);
  assert(0 && "TODO");
}

PortInterface *PortConnectionWrapper::getInterface() {
  if (isEntirePort())
    return port;
  assert(0 && "TODO");
}

PortConnectionManager::
PortConnectionManager(SystemState &sys, PortAliases &aliases) :
  system(sys), portAliases(aliases)
{
}

PortConnectionWrapper PortConnectionManager::get(const PortArg &arg)
{
  Port *p;
  unsigned beginOffset;
  unsigned endOffset;
  bool ok = arg.lookup(system, portAliases, p, beginOffset, endOffset);
  assert(ok);
  // Silence compiler warning.
  (void)ok;
  return PortConnectionWrapper(this, p, beginOffset, endOffset);
}

PortConnectionWrapper PortConnectionManager::
get(const Properties &properties, const std::string &name)
{
  const Property *property = properties.get(name);
  if (!property)
    return PortConnectionWrapper(this, 0, 0, 0);
  return get(property->getAsPort());
}
