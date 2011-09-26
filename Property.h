// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Property_h_
#define _Property_h_

#include <cassert>

#include "PeripheralDescriptor.h"

class Property {
  const PropertyDescriptor *descriptor;
  int32_t integer;
  std::string string;
  uint32_t port;
  Property(const PropertyDescriptor *d) : descriptor(d), integer(0) {}
  void setInteger(int32_t value) {
    assert(descriptor->getType() == PropertyDescriptor::INTEGER);
    integer = value;
  }
  void setString(const std::string &value) {
    assert(descriptor->getType() == PropertyDescriptor::STRING);
    string = value;
  }
  void setPort(const uint32_t value) {
    assert(descriptor->getType() == PropertyDescriptor::PORT);
    port = value;
  }
public:
  static Property integerProperty(const PropertyDescriptor *d, int32_t value);
  static Property stringProperty(const PropertyDescriptor *d,
                                 const std::string &value);
  static Property portProperty(const PropertyDescriptor *d,
                               uint32_t value);
  const PropertyDescriptor *getDescriptor() const { return descriptor; }
  int32_t getAsInteger() const {
    assert(descriptor->getType() == PropertyDescriptor::INTEGER);
    return integer;
  }
  std::string getAsString() const {
    assert(descriptor->getType() == PropertyDescriptor::STRING);
    return string;
  }
  uint32_t getAsPort() const {
    assert(descriptor->getType() == PropertyDescriptor::PORT);
    return port;
  }
};

class Properties {
  std::map<std::string,Property> properties;
public:
  void set(const Property &p) {
    properties.insert(std::make_pair(p.getDescriptor()->getName(), p));
  }
  const Property *get(const std::string &name) const {
    std::map<std::string,Property>::const_iterator it = properties.find(name);
    if (it != properties.end()) {
      return &it->second;
    }
    return 0;
  }
};

#endif //_Property_h_
