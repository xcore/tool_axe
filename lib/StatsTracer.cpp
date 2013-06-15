// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "StatsTracer.h"
#include "llvm/Support/raw_ostream.h"

using namespace axe;

StatsTracer::StatsTracer() :
  numInstructions(0),
  numExceptions(0),
  numEvents(0),
  numInterrupts(0),
  numSyscalls(0)
{
}

StatsTracer::~StatsTracer()
{
  display();
}

void StatsTracer::display()
{
  llvm::raw_ostream &out = llvm::outs();
  out << "Statistics:\n";
  out << "-----------\n";
  out << "Instructions executed: " << numInstructions << '\n';
  // TODO Real time elapsed:
  // TODO Simulator MIPs:
  out << "Number of events: " << numEvents << '\n';
  out << "Number of interrupts " << numInterrupts << '\n';
  out << "Number of exceptions: " << numExceptions << '\n';
  out << "Number of system calls: " << numSyscalls << '\n';
}

void StatsTracer::instructionBegin(const Thread &t)
{
  ++numInstructions;
}

void StatsTracer::exception(const Thread &t, uint32_t et, uint32_t ed,
                 uint32_t sed, uint32_t ssr, uint32_t spc)
{
  ++numExceptions;
}

void StatsTracer::event(const Thread &t, const EventableResource &res, uint32_t pc,
             uint32_t ev)
{
  ++numEvents;
}

void StatsTracer::interrupt(const Thread &t, const EventableResource &res, uint32_t pc,
                 uint32_t ssr, uint32_t spc, uint32_t sed,
                 uint32_t ed)
{
  ++numInterrupts;
}

void StatsTracer::syscall(const Thread &t, const std::string &s)
{
  ++numSyscalls;
}

void StatsTracer::syscall(const Thread &t, const std::string &s, uint32_t op0)
{
  ++numSyscalls;
}
