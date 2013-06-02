// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Trace_h_
#define _Trace_h_

#include "Config.h"
#include "SymbolInfo.h"
#include "Register.h"
#include "TerminalColours.h"
#include <iostream>
#include <sstream>
#include <string>
#include <memory>

namespace axe {

class EventableResource;
class SystemState;
class Node;
class Thread;

inline std::ostream &operator<<(std::ostream &out, const Register::Reg &r) {
  return out << getRegisterName(r);
}

class Tracer {
private:
  bool tracingEnabled;
  std::ostream &out;
  std::ostringstream buf;
  const Thread *thread;
  uint32_t pc;
  bool emittedLineStart;
  size_t numEscapeChars;
  SymbolInfo symInfo;
  TerminalColours colours;

  void escapeCode(const char *s);
  void reset() { escapeCode(colours.reset); }
  void red() { escapeCode(colours.red); }
  void green() { escapeCode(colours.green); }

  void printLinePrefix(const Thread &t);
  void printLinePrefix(const Node &n);
  void printLineEnd();
  void printThreadName(const Thread &t);
  void printThreadPC(const Thread &t, uint32_t pc);
  void printInstructionLineStart(const Thread &t, uint32_t pc);

  void printRegWrite(Register::Reg reg, uint32_t value, bool first);

  void printImm(uint32_t op)
  {
    buf << op;
  }

  void printSrcRegister(Register::Reg op);
  void printDestRegister(Register::Reg op);
  void printSrcDestRegister(Register::Reg op);
  void printCPRelOffset(uint32_t op);
  void printDPRelOffset(uint32_t op);

  void dumpThreadSummary(const Core &core);
  void dumpThreadSummary(const SystemState &system);

  void syscallBegin(const Thread &t);
public:
  Tracer(bool tracing) :
    tracingEnabled(tracing),
    out(std::cout),
    thread(nullptr),
    emittedLineStart(false),
    numEscapeChars(0),
    colours(TerminalColours::null) {}
  bool getTracingEnabled() const { return tracingEnabled; }
  SymbolInfo *getSymbolInfo() { return &symInfo; }
  void setColour(bool enable);

  void instructionBegin(const Thread &t);

  void regWrite(Register::Reg reg, uint32_t value);

  void instructionEnd();

  void SSwitchRead(const Node &node, uint32_t retAddress, uint16_t regNum);
  void SSwitchWrite(const Node &node, uint32_t retAddress, uint16_t regNum,
                    uint32_t value);
  void SSwitchNack(const Node &node, uint32_t dest);
  void SSwitchAck(const Node &node, uint32_t dest);
  void SSwitchAck(const Node &node, uint32_t data, uint32_t dest);

  void exception(const Thread &t, uint32_t et, uint32_t ed,
                 uint32_t sed, uint32_t ssr, uint32_t spc);

  void event(const Thread &t, const EventableResource &res, uint32_t pc,
             uint32_t ev);

  void interrupt(const Thread &t, const EventableResource &res, uint32_t pc,
                 uint32_t ssr, uint32_t spc, uint32_t sed, uint32_t ed);

  void syscall(const Thread &t, const std::string &s) {
    syscallBegin(t);
    buf << s << "()";
    reset();
    printLineEnd();
  }
  template<typename T0>
  void syscall(const Thread &t, const std::string &s,
               T0 op0) {
    syscallBegin(t);
    buf << s << '(' << op0 << ')';
    reset();
    printLineEnd();
  }
  void timeout(const SystemState &system, ticks_t time);
  void noRunnableThreads(const SystemState &system);
};

} // End axe namespace

#endif //_Trace_h_
