// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _ProcessorNode_h
#define _ProcessorNode_h

#include <vector>
#include "Node.h"

namespace axe {

class Core;

class ProcessorNode : public Node {
private:
  std::vector<Core *> cores;
  
  void computeCoreNumberBits();
public:
  bool isProcessorNode() override { return true; }
  typedef std::vector<Core *>::iterator core_iterator;
  typedef std::vector<Core *>::const_iterator const_core_iterator;
  ProcessorNode(Type t, unsigned numXLinks);
  ProcessorNode(const Node &) = delete;
  ~ProcessorNode();
  void finalize() override;
  void addCore(std::unique_ptr<Core> cores);
  const std::vector<Core*> &getCores() const { return cores; }
  uint32_t getCoreID(unsigned coreNum) const;
  void setNodeID(unsigned value) override;
  static bool getTypeFromJtagID(unsigned jtagID, Type &type);
  ChanEndpoint *getLocalChanendDest(ResourceID ID) override;
};

} // End axe namespace

#endif // _ProcessorNode_h
