// Copyright (c) 2011-2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Trace_h_
#define _Trace_h_

#ifndef __GNU_SOURCE
#define __GNU_SOURCE
#endif
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include "Config.h"
#include "Register.h"
#include <string>

namespace axe {

class EventableResource;
class SystemState;
class Node;
class Thread;

class Tracer {
public:
  virtual ~Tracer();

  virtual void instructionBegin(const Thread &t);

  virtual void regWrite(Register::Reg reg, uint32_t value);

  virtual void instructionEnd();

  virtual void SSwitchRead(const Node &node, uint32_t retAddress,
                           uint16_t regNum);
  virtual void SSwitchWrite(const Node &node, uint32_t retAddress,
                            uint16_t regNum, uint32_t value);
  virtual void SSwitchNack(const Node &node, uint32_t dest);
  virtual void SSwitchAck(const Node &node, uint32_t dest);
  virtual void SSwitchAck(const Node &node, uint32_t data, uint32_t dest);

  virtual void exception(const Thread &t, uint32_t et, uint32_t ed,
                         uint32_t sed, uint32_t ssr, uint32_t spc);

  virtual void event(const Thread &t, const EventableResource &res, uint32_t pc,
                     uint32_t ev);

  virtual void interrupt(const Thread &t, const EventableResource &res,
                         uint32_t pc, uint32_t ssr, uint32_t spc, uint32_t sed,
                         uint32_t ed);

  virtual void syscall(const Thread &t, const std::string &s);
  // TODO provide a more sensible interface.
  virtual void syscall(const Thread &t, const std::string &s, uint32_t op0);
  virtual void timeout(const SystemState &system, ticks_t time);
  virtual void noRunnableThreads(const SystemState &system);
};

} // End axe namespace

#endif //_Trace_h_
