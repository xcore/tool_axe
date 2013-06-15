// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "DelegatingTracer.h"

using namespace axe;

DelegatingTracer::~DelegatingTracer()
{
  for (Tracer *tracer : delegates) {
    delete tracer;
  }
}

void DelegatingTracer::addDelegate(std::auto_ptr<Tracer> tracer)
{
  delegates.push_back(tracer.release());
}

void DelegatingTracer::attach(const SystemState &systemState)
{
  for (Tracer *tracer : delegates) {
    tracer->attach(systemState);
  }
}

void DelegatingTracer::instructionBegin(const Thread &t)
{
  for (Tracer *tracer : delegates) {
    tracer->instructionBegin(t);
  }
}

void DelegatingTracer::regWrite(Register::Reg reg, uint32_t value)
{
  for (Tracer *tracer : delegates) {
    tracer->regWrite(reg, value);
  }
}

void DelegatingTracer::instructionEnd()
{
  for (Tracer *tracer : delegates) {
    tracer->instructionEnd();
  }
}

void DelegatingTracer::
SSwitchRead(const Node &node, uint32_t retAddress, uint16_t regNum)
{
  for (Tracer *tracer : delegates) {
    tracer->SSwitchRead(node, retAddress, regNum);
  }
}

void DelegatingTracer::
SSwitchWrite(const Node &node, uint32_t retAddress, uint16_t regNum,
             uint32_t value)
{
  for (Tracer *tracer : delegates) {
    tracer->SSwitchWrite(node, retAddress, regNum, value);

  }
}

void DelegatingTracer::SSwitchNack(const Node &node, uint32_t dest)
{
  for (Tracer *tracer : delegates) {
    tracer->SSwitchNack(node, dest);
  }
}

void DelegatingTracer::SSwitchAck(const Node &node, uint32_t dest)
{
  for (Tracer *tracer : delegates) {
    tracer->SSwitchAck(node, dest);
  }
}

void DelegatingTracer::
SSwitchAck(const Node &node, uint32_t data,
           uint32_t dest)
{
  for (Tracer *tracer : delegates) {
    tracer->SSwitchAck(node, data, dest);
  }
}

void DelegatingTracer::
exception(const Thread &t, uint32_t et, uint32_t ed, uint32_t sed, uint32_t ssr,
          uint32_t spc)
{
  for (Tracer *tracer : delegates) {
    tracer->exception(t, et, ed, sed, ssr, spc);
  }
}

void DelegatingTracer::
event(const Thread &t, const EventableResource &res,
      uint32_t pc, uint32_t ev)
{
  for (Tracer *tracer : delegates) {
    tracer->event(t, res, pc, ev);
  }
}

void DelegatingTracer::
interrupt(const Thread &t, const EventableResource &res, uint32_t pc,
          uint32_t ssr, uint32_t spc, uint32_t sed, uint32_t ed)
{
  for (Tracer *tracer : delegates) {
    tracer->interrupt(t, res, pc, ssr, spc, sed, ed);
  }
}

void DelegatingTracer::syscall(const Thread &t, const std::string &s)
{
  for (Tracer *tracer : delegates) {
    tracer->syscall(t, s);
  }
}

void DelegatingTracer::
syscall(const Thread &t, const std::string &s,
        uint32_t op0)
{
  for (Tracer *tracer : delegates) {
    tracer->syscall(t, s, op0);
  }
}

void DelegatingTracer::timeout(const SystemState &system, ticks_t time)
{
  for (Tracer *tracer : delegates) {
    tracer->timeout(system, time);
  }
}

void DelegatingTracer::noRunnableThreads(const SystemState &system)
{
  for (Tracer *tracer : delegates) {
    tracer->noRunnableThreads(system);
  }
}
