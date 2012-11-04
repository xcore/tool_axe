// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Trace_h_
#define _Trace_h_

#include "SymbolInfo.h"
#include "Thread.h"
#include "TerminalColours.h"
#include <iostream>
#include <sstream>
#include <string>
#include <memory>

namespace axe {

class EventableResource;
class SystemState;
class Node;

inline std::ostream &operator<<(std::ostream &out, const Register::Reg &r) {
  return out << getRegisterName(r);
}

class SrcRegister {
  Register::Reg reg;
public:
  SrcRegister(Register::Reg r) : reg(r) {}
  SrcRegister(unsigned r) : reg((Register::Reg)r) {}
  Register::Reg getRegister() const { return reg; }
};

class DestRegister {
  Register::Reg reg;
public:
  DestRegister(Register::Reg r) : reg(r) {}
  DestRegister(unsigned r) : reg((Register::Reg)r) {}
  Register::Reg getRegister() const { return reg; }
};

class SrcDestRegister {
  Register::Reg reg;
public:
  SrcDestRegister(Register::Reg r) : reg(r) {}
  SrcDestRegister(unsigned r) : reg((Register::Reg)r) {}
  Register::Reg getRegister() const { return reg; }
};

class CPRelOffset {
  uint32_t offset;
public:
  CPRelOffset(uint32_t o) : offset(o) {}
  uint32_t getOffset() const { return offset; }
};

class DPRelOffset {
  uint32_t offset;
public:
  DPRelOffset(uint32_t o) : offset(o) {}
  uint32_t getOffset() const { return offset; }
};

class Tracer {
private:
  struct LineState {
    LineState(std::ostringstream *b) :
      thread(0),
      out(0),
      buf(b),
      pending(0) {}
    LineState(std::ostream &o, std::ostringstream &b, std::ostringstream &pendingBuf) :
      thread(0),
      out(&o),
      buf(&b),
      pending(&pendingBuf) {}
    const Thread *thread;
    bool hadRegWrite;
    size_t numEscapeChars;
    std::ostream *out;
    std::ostringstream *buf;
    std::ostringstream *pending;
  };
  class PushLineState {
  private:
    bool needRestore;
    LineState line;
    Tracer &parent;
  public:
    PushLineState(Tracer &parent);
    ~PushLineState();
    bool getRestore() const { return needRestore; }
  };
  bool tracingEnabled;
  std::ostringstream buf;
  std::ostringstream pendingBuf;
  LineState line;
  SymbolInfo symInfo;
  TerminalColours colours;

  void escapeCode(const char *s);
  void reset() { escapeCode(colours.reset); }
  void red() { escapeCode(colours.red); }
  void green() { escapeCode(colours.green); }

  void printThreadName();
  void printCommonStart();
  void printCommonStart(const Node &n);
  void printCommonStart(const Thread &t);
  void printCommonEnd();
  void printThreadPC();
  void printInstructionStart(const Thread &t);

  template <typename T>
    void printOperand(const T &op)
  {
    *line.buf << op;
  }

  void printOperand(SrcRegister op);
  void printOperand(DestRegister op);
  void printOperand(SrcDestRegister op);
  void printOperand(CPRelOffset op);
  void printOperand(DPRelOffset op);

  void dumpThreadSummary(const Core &core);
  void dumpThreadSummary(const SystemState &system);
  
  void traceAux() { }
  template<typename T, typename... Args>
  void traceAux(T op0, Args... args)
  {
    printOperand(op0);
    traceAux(args...);
  }
public:
  Tracer(bool tracing) :
    tracingEnabled(tracing),
    line(std::cout, buf, pendingBuf),
    colours(TerminalColours::null) {}
  bool getTracingEnabled() const { return tracingEnabled; }
  SymbolInfo *getSymbolInfo() { return &symInfo; }
  void setColour(bool enable);

  template<typename... Args>
  void trace(const Thread &t, Args... args)
  {
    printInstructionStart(t);
    traceAux(args...);
  }

  void regWrite(Register::Reg reg, uint32_t value);

  void traceEnd() {
    printCommonEnd();
  }

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

  void syscallBegin(const Thread &t);

  void syscall(const Thread &t, const std::string &s) {
    syscallBegin(t);
    *line.buf << s << "()";
    reset();
  }
  template<typename T0>
  void syscall(const Thread &t, const std::string &s,
               T0 op0) {
    syscallBegin(t);
    *line.buf << s << '(' << op0 << ')';
    reset();
  }
  void syscallEnd() {
    printCommonEnd();
  }

  void noRunnableThreads(const SystemState &system);
};
  
} // End axe namespace

#endif //_Trace_h_
