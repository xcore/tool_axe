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

class SystemState;

class Node;
class ProcessorNode;

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
  Type type;
  std::vector<XLink> xLinks;
  std::vector<uint8_t> directions;
  unsigned jtagIndex;
  unsigned nodeID;
  SystemState *parent;
  SSwitch sswitch;
  unsigned nodeNumberBits;
  XLink *getXLinkForDirection(unsigned direction);
protected:
  void setNodeNumberBits(unsigned value);
public:
  Node(Type type, unsigned numXLinks);
  Node(const Node &) = delete;
  virtual ~Node();
  virtual bool isProcessorNode() { return false; }
  unsigned getNodeNumberBits() const;
  unsigned getNonNodeNumberBits() const;
  virtual void finalize();
  void setJtagIndex(unsigned value) { jtagIndex = value; }
  unsigned getJtagIndex() const { return jtagIndex; }
  void setParent(SystemState *value) { parent = value; }
  const SystemState *getParent() const { return parent; }
  SystemState *getParent() { return parent; }
  virtual void setNodeID(unsigned value);
  uint32_t getNodeID() const { return nodeID; }
  bool hasMatchingNodeID(ResourceID ID);
  SSwitch *getSSwitch() { return &sswitch; }
  unsigned getNumXLinks() const { return xLinks.size(); }
  XLink &getXLink(unsigned num) { return xLinks[num]; }
  const XLink &getXLink(unsigned num) const { return xLinks[num]; }
  void connectXLink(unsigned num, Node *destNode, unsigned destNum);
  ChanEndpoint *getChanendDest(ResourceID ID);
  virtual ChanEndpoint *getLocalChanendDest(ResourceID ID) = 0;
  uint8_t getDirection(unsigned num) const { return directions[num]; }
  void setDirection(unsigned num, uint8_t value) { directions[num] = value; }
  Type getType() const { return type; }
};
  
} // End axe namespace

#endif // _Node_h_
