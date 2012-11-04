// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PeripheralDescriptor.h"

using namespace axe;

PropertyDescriptor &PeripheralDescriptor::
addProperty(const PropertyDescriptor &p)
{
  return properties.insert(std::make_pair(p.getName(), p)).first->second;
}

const PropertyDescriptor *PeripheralDescriptor::
getProperty(const std::string &name) const
{
  auto it = properties.find(name);
  if (it == properties.end())
    return 0;
  return &it->second;
}

PropertyDescriptor PropertyDescriptor::integerProperty(const std::string &name)
{
  return PropertyDescriptor(INTEGER, name);
}

PropertyDescriptor PropertyDescriptor::stringProperty(const std::string &name)
{
  return PropertyDescriptor(STRING, name);
}

PropertyDescriptor PropertyDescriptor::portProperty(const std::string &name)
{
  return PropertyDescriptor(PORT, name);
}
