// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _BootSequence_h
#define _BootSequence_h

#include <vector>
#include <stdint.h>

namespace axe {

class Core;
class SystemState;
class XEElfSector;
class BootSequenceStep;

class BootSequence {
  SystemState &sys;
  std::vector<BootSequenceStep*> steps;
public:
  BootSequence(SystemState &s) : sys(s) {}
  ~BootSequence();
  void addElf(Core *c, const XEElfSector *elfSector);
  void addSchedule(Core *c, uint32_t address);
  void addRun(unsigned numDoneSyscalls);
  void overrideEntryPoint(uint32_t address);
  void eraseAllButLastImage();
  void setLoadImages(bool value);
  int execute();
  /// Initialize ELF handling global state. Normally this state is initialized
  /// when it is first used but in a multi-threaded applications it may need to
  /// be initialized earier to prevent avoid race conditions.
  static void initializeElfHandling();
};

} // End axe namespace

#endif // _BootSequence_h
