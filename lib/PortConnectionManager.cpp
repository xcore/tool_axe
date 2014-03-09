// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PortConnectionManager.h"
#include "PortArg.h"
#include "Port.h"
#include "Property.h"
#include "PortSplitter.h"
#include "PortCombiner.h"
#include "SystemState.h"

using namespace axe;

void PortConnectionWrapper::attach(PortInterface *to) {
  parent->attach(to, port, beginOffset, endOffset);
}

PortInterface *PortConnectionWrapper::getInterface() {
  return parent->getInterface(port, beginOffset, endOffset);
}

PortConnectionManager::
PortConnectionManager(SystemState &sys, PortAliases &aliases) :
  system(sys), portAliases(aliases)
{
}

PortConnectionManager::~PortConnectionManager()
{
  for (auto &entry : splitters) {
    delete entry.second;
  }
  for (auto &entry : combiners) {
    delete entry.second;
  }
}

bool PortConnectionManager::
isEntirePort(Port *p, unsigned beginOffset, unsigned endOffset) const
{
  if (!p)
    return true;
  return beginOffset == 0 && endOffset == p->getPortWidth();
}

void PortConnectionManager::
attach(PortInterface *to, Port *from, unsigned beginOffset, unsigned endOffset)
{
  if (!from->getLoopback() && isEntirePort(from, beginOffset, endOffset)) {
    from->setLoopback(to);
    return;
  }
  PortCombiner *&combiner = combiners[from];
  if (!combiner) {
    combiner = new PortCombiner(system.getScheduler());
    PortInterface *old = from->getLoopback();
    from->setLoopback(combiner);
    if (old) {
      combiner->attach(old, 0, from->getPortWidth());
    }
  }
  combiner->attach(to, beginOffset, endOffset);
}

PortInterface *PortConnectionManager::
getInterface(Port *p, unsigned beginOffset, unsigned endOffset)
{
  if (isEntirePort(p, beginOffset, endOffset))
    return p;
  PortSplitter *&splitter = splitters[p];
  if (!splitter) {
    splitter = new PortSplitter(system.getScheduler(), p);
  }
  return splitter->getInterface(beginOffset, endOffset);
}

PortConnectionWrapper PortConnectionManager::get(const PortArg &arg)
{
  Port *p;
  unsigned beginOffset;
  unsigned endOffset;
  if (!arg.lookup(system, portAliases, p, beginOffset, endOffset))
    return PortConnectionWrapper(this, 0, 0, 0);
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
