// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Property.h"

using namespace axe;

int32_t Property::getAsInteger() const {
  assert(descriptor->getType() == PropertyDescriptor::INTEGER);
  return static_cast<const IntegerProperty*>(this)->getValue();
}
std::string Property::getAsString() const {
  assert(descriptor->getType() == PropertyDescriptor::STRING);
  return static_cast<const StringProperty*>(this)->getValue();
}
const PortArg &Property::getAsPort() const {
  assert(descriptor->getType() == PropertyDescriptor::PORT);
  return static_cast<const PortProperty*>(this)->getValue();
}

Properties::~Properties()
{
  for (const auto &entry : properties) {
    delete entry.second;
  }
}
