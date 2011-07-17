// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Node_h_
#define _Node_h_

#include <stdint.h>
#include <vector>
#include <memory>

class Core;
class SystemState;

class Node {
public:
  enum Type {
    XS1_L,
    XS1_G
  };
  std::vector<Core *> cores;
  unsigned jtagIndex;
  unsigned nodeID;
  SystemState *parent;
  Type type;
public:
  Node(Type t) : jtagIndex(0), nodeID(0), parent(0), type(t) {}
  ~Node();
  void addCore(std::auto_ptr<Core> cores);
  void setJtagIndex(unsigned value) { jtagIndex = value; }
  unsigned getJtagIndex() const { return jtagIndex; }
  const std::vector<Core*> &getCores() const { return cores; }
  void setParent(SystemState *value) { parent = value; }
  const SystemState *getParent() const { return parent; }
  SystemState *getParent() { return parent; }
  void setNodeID(unsigned value);
  uint32_t getNodeID() const { return nodeID; }
  Type getType() const { return type; }
  static bool getTypeFromJtagID(unsigned jtagID, Type &type);
};

#endif // _Node_h_
