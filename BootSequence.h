// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _BootSequence_h
#define _BootSequence_h

#include <vector>

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
};

#endif // _BootSequence_h
