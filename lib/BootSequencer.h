// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _BootSequencer_h
#define _BootSequencer_h

#include <vector>
#include <stdint.h>
#include "BreakpointManager.h"

namespace axe {

class Core;
class SyscallHandler;
class SystemState;
class XEElfSector;
class BootSequenceStep;
class XE;

class BootSequencer {
  SystemState &sys;
  BreakpointManager breakpointManager;
  SyscallHandler *syscallHandler;
  std::vector<BootSequenceStep*> steps;
  void setEntryPointToRom();
  void eraseAllButLastImage();
  void setLoadImages(bool value);
public:
  BootSequencer(SystemState &s);
  BootSequencer(const BootSequencer &) = delete;
  ~BootSequencer();
  void addElf(Core *c, const XEElfSector *elfSector);
  void addSchedule(Core *c, uint32_t address);
  void addRun(unsigned numDoneSyscalls);
  void populateFromXE(XE &xe);
  void adjustForSPIBoot();
  int execute();
  /// Initialize ELF handling global state. Normally this state is initialized
  /// when it is first used but in a multi-threaded applications it may need to
  /// be initialized earier to prevent avoid race conditions.
  static void initializeElfHandling();
};

} // End axe namespace

#endif // _BootSequencer_h
