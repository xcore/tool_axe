// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Property.h"

Property Property::integerProperty(const PropertyDescriptor *d, int32_t value)
{
  Property p(d);
  p.setInteger(value);
  return p;
}

Property Property::
stringProperty(const PropertyDescriptor *d, const std::string &value)
{
  Property p(d);
  p.setString(value);
  return p;
}

Property Property::
portProperty(const PropertyDescriptor *d, uint32_t value)
{
  Property p(d);
  p.setPort(value);
  return p;
}
