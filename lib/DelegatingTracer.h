// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _DelegatingTracer_h
#define _DelegatingTracer_h

#include "Tracer.h"
#include <memory>
#include <vector>

namespace axe {
  class DelegatingTracer : public Tracer {
    std::vector<Tracer*> delegates;
  public:
    ~DelegatingTracer();

    void addDelegate(std::unique_ptr<Tracer> tracer);

    virtual void attach(const SystemState &systemState) override;

    virtual void instructionBegin(const Thread &t) override;

    virtual void regWrite(Register::Reg reg, uint32_t value) override;

    virtual void instructionEnd() override;

    virtual void SSwitchRead(const Node &node, uint32_t retAddress,
                             uint16_t regNum) override;
    virtual void SSwitchWrite(const Node &node, uint32_t retAddress,
                              uint16_t regNum, uint32_t value) override;
    virtual void SSwitchNack(const Node &node, uint32_t dest) override;
    virtual void SSwitchAck(const Node &node, uint32_t dest) override;
    virtual void SSwitchAck(const Node &node, uint32_t data,
                            uint32_t dest) override;

    virtual void exception(const Thread &t, uint32_t et, uint32_t ed,
                           uint32_t sed, uint32_t ssr, uint32_t spc)  override;

    virtual void event(const Thread &t, const EventableResource &res,
                       uint32_t pc, uint32_t ev) override;

    virtual void interrupt(const Thread &t, const EventableResource &res,
                           uint32_t pc, uint32_t ssr, uint32_t spc, uint32_t sed,
                           uint32_t ed) override;

    virtual void syscall(const Thread &t, const std::string &s) override;
    virtual void syscall(const Thread &t, const std::string &s,
                         uint32_t op0) override;
    virtual void timeout(const SystemState &system, ticks_t time) override;
    virtual void noRunnableThreads(const SystemState &system) override;
  };
} // End axe namespace

#endif // _DelegatingTracer_h
