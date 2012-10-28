// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PeripheralDescriptor_h_
#define _PeripheralDescriptor_h_

#include <cassert>
#include <map>
#include <string>
#include "AccessSecondIterator.h"

namespace axe {

class Peripheral;
class Properties;
class PortConnectionManager;
class SystemState;

class PropertyDescriptor {
public:
  enum Type {
    INTEGER,
    STRING,
    PORT,
  };
private:
  Type type;
  std::string name;
  bool required;
public:
  PropertyDescriptor(Type t, const std::string &n) :
    type(t),
    name(n),
    required(false) {}
  static PropertyDescriptor integerProperty(const std::string &name);
  static PropertyDescriptor stringProperty(const std::string &name);
  static PropertyDescriptor portProperty(const std::string &name);
  const std::string &getName() const { return name; }
  Type getType() const { return type; }
  bool getRequired() const { return required; }
  PropertyDescriptor &setRequired(bool value) {
    required = value;
    return *this;
  }
};

class PeripheralDescriptor {
  std::string name;
  std::map<std::string,PropertyDescriptor> properties;
  typedef Peripheral*(*CreateFunc)(SystemState &, PortConnectionManager &,
                                   const Properties &);
  CreateFunc create;
public:
  PeripheralDescriptor(const std::string &n, CreateFunc c) :
    name(n),
    create(c)
  {
  }
  const std::string &getName() const { return name; }
  PropertyDescriptor &addProperty(const PropertyDescriptor &p);
  const PropertyDescriptor *getProperty(const std::string &name) const;
  void createInstance(SystemState &system,
                      PortConnectionManager &connectionManager,
                      const Properties &properties) const {
    (*create)(system, connectionManager, properties);
  }
  typedef AccessSecondIterator<std::map<std::string,PropertyDescriptor>::iterator> iterator;
  typedef AccessSecondIterator<std::map<std::string,PropertyDescriptor>::const_iterator> const_iterator;
  iterator properties_begin() { return properties.begin(); }
  iterator properties_end() { return properties.end(); }
  const_iterator properties_begin() const { return properties.begin(); }
  const_iterator properties_end() const { return properties.end(); }
};
  
} // End axe namespace

#endif // _PeripheralDescriptor_h_
