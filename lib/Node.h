// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Node_h_
#define _Node_h_

#include <stdint.h>
#include <vector>
#include <memory>
#include "SSwitch.h"
#include "Resource.h"

namespace axe {

class Core;
class SystemState;

class Node;

class XLink {
  friend class Node;
  Node *destNode;
  unsigned destXLinkNum;
  bool enabled;
  bool fiveWire;
  uint8_t network;
  uint8_t direction;
  uint16_t interTokenDelay;
  uint16_t interSymbolDelay;
public:
  XLink();
  const XLink *getDestXLink() const;
  void setEnabled(bool value) { enabled = value; }
  bool isEnabled() const { return enabled; }
  void setFiveWire(bool value) { fiveWire = value; }
  bool isFiveWire() const { return fiveWire; }
  void setNetwork(uint8_t value) { network = value; }
  uint8_t getNetwork() const { return network; }
  void setDirection(uint8_t value) { direction = value; }
  uint8_t getDirection() const { return network; }
  void setInterTokenDelay(uint16_t value) { interTokenDelay = value; }
  uint16_t getInterTokenDelay() const { return interTokenDelay; }
  void setInterSymbolDelay(uint16_t value) { interSymbolDelay = value; }
  uint16_t getInterSymbolDelay() const { return interSymbolDelay; }
  bool isConnected() const;
};

class Node {
public:
  enum Type {
    XS1_L,
    XS1_G
  };
private:
  std::vector<Core *> cores;
  std::vector<XLink> xLinks;
  std::vector<uint8_t> directions;
  unsigned jtagIndex;
  unsigned nodeID;
  SystemState *parent;
  Type type;
  SSwitch sswitch;
  unsigned coreNumberBits;

  void computeCoreNumberBits();
  unsigned getCoreNumberBits() const;
  XLink *getXLinkForDirection(unsigned direction);
public:
  typedef std::vector<Core *>::iterator core_iterator;
  typedef std::vector<Core *>::const_iterator const_core_iterator;
  Node(Type t, unsigned numXLinks);
  Node(const Node &) = delete;
  ~Node();
  unsigned getNodeNumberBits() const;
  void finalize();
  void addCore(std::auto_ptr<Core> cores);
  void setJtagIndex(unsigned value) { jtagIndex = value; }
  unsigned getJtagIndex() const { return jtagIndex; }
  const std::vector<Core*> &getCores() const { return cores; }
  void setParent(SystemState *value) { parent = value; }
  const SystemState *getParent() const { return parent; }
  SystemState *getParent() { return parent; }
  void setNodeID(unsigned value);
  uint32_t getNodeID() const { return nodeID; }
  uint32_t getCoreID(unsigned coreNum) const;
  Type getType() const { return type; }
  static bool getTypeFromJtagID(unsigned jtagID, Type &type);
  bool hasMatchingNodeID(ResourceID ID);
  SSwitch *getSSwitch() { return &sswitch; }
  unsigned getNumXLinks() const { return xLinks.size(); }
  XLink &getXLink(unsigned num) { return xLinks[num]; }
  const XLink &getXLink(unsigned num) const { return xLinks[num]; }
  void connectXLink(unsigned num, Node *destNode, unsigned destNum);
  ChanEndpoint *getChanendDest(ResourceID ID);
  uint8_t getDirection(unsigned num) const { return directions[num]; }
  void setDirection(unsigned num, uint8_t value) { directions[num] = value; }
};
  
} // End axe namespace

#endif // _Node_h_
