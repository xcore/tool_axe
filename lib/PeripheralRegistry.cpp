// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PeripheralRegistry.h"
#include "PeripheralDescriptor.h"
#include <map>

using namespace axe;

std::map<std::string,PeripheralDescriptor*> peripherals;

void PeripheralRegistry::add(std::unique_ptr<PeripheralDescriptor> p)
{
  // TODO fix memory leak.
  std::string name = p->getName();
  peripherals.insert(std::make_pair(name, p.release()));
}

PeripheralDescriptor *PeripheralRegistry::get(const std::string &name)
{
  auto it = peripherals.find(name);
  if (it == peripherals.end())
    return 0;
  return it->second;
}

PeripheralRegistry::iterator PeripheralRegistry::begin()
{
  return iterator(peripherals.begin());
}

PeripheralRegistry::iterator PeripheralRegistry::end()
{
  return iterator(peripherals.end());
}
