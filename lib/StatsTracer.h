// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _StatsTracer_h
#define _StatsTracer_h

#include "Tracer.h"

namespace axe {
  class StatsTracer : public Tracer {
    uint64_t numInstructions;
    uint64_t numExceptions;
    uint64_t numEvents;
    uint64_t numInterrupts;
    uint64_t numSyscalls;
  
    void display(void);
  public:
    StatsTracer();
    ~StatsTracer();

    void instructionBegin(const Thread &t) override;
    void exception(const Thread &t, uint32_t et, uint32_t ed,
                   uint32_t sed, uint32_t ssr, uint32_t spc) override;
    void event(const Thread &t, const EventableResource &res, uint32_t pc,
               uint32_t ev) override;
    void interrupt(const Thread &t, const EventableResource &res, uint32_t pc,
                   uint32_t ssr, uint32_t spc, uint32_t sed,
                   uint32_t ed) override;
    void syscall(const Thread &t, const std::string &s) override;
    // TODO provide a more sensible interface.
    void syscall(const Thread &t, const std::string &s, uint32_t op0) override;
  };
} // End axe namespace

#endif // _StatsTracer_h
