// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Property_h_
#define _Property_h_

#include <cassert>

#include "PeripheralDescriptor.h"
#include "PortArg.h"
#include <stdint.h>

namespace axe {

class Property {
  const PropertyDescriptor *descriptor;
protected:
  Property(const PropertyDescriptor *d) : descriptor(d) {}
public:
  const PropertyDescriptor *getDescriptor() const { return descriptor; }
  int32_t getAsInteger() const;
  std::string getAsString() const;
  const PortArg &getAsPort() const;
};

template <typename T>
class ConcreteProperty : public Property {
  T value;
public:
  ConcreteProperty(const PropertyDescriptor *d, const T &v) :
    Property(d), value(v) {}

  const T &getValue() const { return value; }
};

typedef ConcreteProperty<int32_t> IntegerProperty;
typedef ConcreteProperty<std::string> StringProperty;
typedef ConcreteProperty<PortArg> PortProperty;

class Properties {
  std::map<std::string,Property*> properties;
public:
  ~Properties();
  Properties() = default;
  Properties(const Properties &other) = delete;
  void setIntegerProperty(const PropertyDescriptor *d, int32_t value) {
    properties.insert(std::make_pair(d->getName(),
                                     new IntegerProperty(d, value)));
  }
  void setStringProperty(const PropertyDescriptor *d,
                         const std::string &value) {
    properties.insert(std::make_pair(d->getName(),
                                     new StringProperty(d, value)));
  }
  void setPortProperty(const PropertyDescriptor *d, const PortArg &value) {
    properties.insert(std::make_pair(d->getName(),
                                     new PortProperty(d, value)));
  }
  const Property *get(const std::string &name) const {
    auto it = properties.find(name);
    if (it != properties.end()) {
      return it->second;
    }
    return 0;
  }
};
  
} // End axe namespace

#endif //_Property_h_
