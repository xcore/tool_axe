// Copyright (c) 2011-2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Tracer.h"

using namespace axe;

Tracer::~Tracer()
{
}

void Tracer::attach(const SystemState &systemState)
{
}

void Tracer::instructionBegin(const Thread &t)
{
}

void Tracer::regWrite(Register::Reg reg, uint32_t value)
{
}

void Tracer::instructionEnd()
{
}

void Tracer::SSwitchRead(const Node &node, uint32_t retAddress,
                         uint16_t regNum)
{
}

void Tracer::SSwitchWrite(const Node &node, uint32_t retAddress,
                          uint16_t regNum, uint32_t value)
{
}

void Tracer::SSwitchNack(const Node &node, uint32_t dest)
{
}

void Tracer::SSwitchAck(const Node &node, uint32_t dest)
{
}

void Tracer::SSwitchAck(const Node &node, uint32_t data, uint32_t dest)
{
}

void Tracer::exception(const Thread &t, uint32_t et, uint32_t ed,
                       uint32_t sed, uint32_t ssr, uint32_t spc)
{
}

void Tracer::event(const Thread &t, const EventableResource &res, uint32_t pc,
                   uint32_t ev)
{
}

void Tracer::interrupt(const Thread &t, const EventableResource &res,
                       uint32_t pc, uint32_t ssr, uint32_t spc, uint32_t sed,
                       uint32_t ed)
{
}

void Tracer::syscall(const Thread &t, const std::string &s)
{
}

void Tracer::syscall(const Thread &t, const std::string &s, uint32_t op0)
{
}

void Tracer::timeout(const SystemState &system, ticks_t time)
{
}

void Tracer::noRunnableThreads(const SystemState &system)
{
}
