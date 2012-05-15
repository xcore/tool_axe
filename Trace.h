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
  Tracer() :
    tracingEnabled(false),
    line(std::cout, buf, pendingBuf),
    colours(TerminalColours::null) {}
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
  public:
    PushLineState();
    ~PushLineState();
    bool getRestore() const { return needRestore; }
  };
  bool tracingEnabled;
  std::ostringstream buf;
  std::ostringstream pendingBuf;
  LineState line;
  std::auto_ptr<SymbolInfo> symInfo;
  TerminalColours colours;

  static Tracer instance;

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
public:

  void setTracingEnabled(bool enable) { tracingEnabled = enable; }
  bool getTracingEnabled() const { return tracingEnabled; }
  void setSymbolInfo(std::auto_ptr<SymbolInfo> &si);
  SymbolInfo *getSymbolInfo() { return symInfo.get(); }
  void setColour(bool enable);

  template<typename T0>
  void trace(const Thread &t, T0 op0)
  {
    printInstructionStart(t);
    printOperand(op0);
  }

  template<typename T0, typename T1>
  void trace(const Thread &t, T0 op0, T1 op1)
  {
    printInstructionStart(t);
    printOperand(op0);
    printOperand(op1);
  }

  template<typename T0, typename T1, typename T2>
  void trace(const Thread &t, T0 op0, T1 op1, T2 op2)
  {
    printInstructionStart(t);
    printOperand(op0);
    printOperand(op1);
    printOperand(op2);
  }

  template<typename T0, typename T1, typename T2, typename T3>
  void trace(const Thread &t, T0 op0, T1 op1, T2 op2,
             T3 op3)
  {
    printInstructionStart(t);
    printOperand(op0);
    printOperand(op1);
    printOperand(op2);
    printOperand(op3);
  }

  template<typename T0, typename T1, typename T2, typename T3, typename T4>
  void trace(const Thread &t, T0 op0, T1 op1, T2 op2, T3 op3, T4 op4)
  {
    printInstructionStart(t);
    printOperand(op0);
    printOperand(op1);
    printOperand(op2);
    printOperand(op3);
    printOperand(op4);
  }

  template<typename T0, typename T1, typename T2, typename T3, typename T4,
  typename T5>
  void trace(const Thread &t, T0 op0, T1 op1, T2 op2, T3 op3, T4 op4,
             T5 op5)
  {
    printInstructionStart(t);
    printOperand(op0);
    printOperand(op1);
    printOperand(op2);
    printOperand(op3);
    printOperand(op4);
    printOperand(op5);
  }

  template<typename T0, typename T1, typename T2, typename T3, typename T4,
           typename T5, typename T6>
  void trace(const Thread &t, T0 op0, T1 op1, T2 op2, T3 op3, T4 op4,
             T5 op5, T6 op6)
  {
    printInstructionStart(t);
    printOperand(op0);
    printOperand(op1);
    printOperand(op2);
    printOperand(op3);
    printOperand(op4);
    printOperand(op5);
    printOperand(op6);
  }

  template<typename T0, typename T1, typename T2, typename T3, typename T4,
           typename T5, typename T6, typename T7>
  void trace(const Thread &t, T0 op0, T1 op1, T2 op2, T3 op3, T4 op4,
             T5 op5, T6 op6, T7 op7)
  {
    printInstructionStart(t);
    printOperand(op0);
    printOperand(op1);
    printOperand(op2);
    printOperand(op3);
    printOperand(op4);
    printOperand(op5);
    printOperand(op6);
    printOperand(op7);
  }

  template<typename T0, typename T1, typename T2, typename T3, typename T4,
  typename T5, typename T6, typename T7, typename T8>
  void trace(const Thread &t, T0 op0, T1 op1, T2 op2, T3 op3, T4 op4,
             T5 op5, T6 op6, T7 op7, T8 op8)
  {
    printInstructionStart(t);
    printOperand(op0);
    printOperand(op1);
    printOperand(op2);
    printOperand(op3);
    printOperand(op4);
    printOperand(op5);
    printOperand(op6);
    printOperand(op7);
    printOperand(op8);
  }

  template<typename T0, typename T1, typename T2, typename T3, typename T4,
  typename T5, typename T6, typename T7, typename T8, typename T9>
  void trace(const Thread &t, T0 op0, T1 op1, T2 op2, T3 op3, T4 op4,
             T5 op5, T6 op6, T7 op7, T8 op8, T9 op9)
  {
    printInstructionStart(t);
    printOperand(op0);
    printOperand(op1);
    printOperand(op2);
    printOperand(op3);
    printOperand(op4);
    printOperand(op5);
    printOperand(op6);
    printOperand(op7);
    printOperand(op8);
    printOperand(op9);
  }

  template<typename T0, typename T1, typename T2, typename T3, typename T4,
  typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
  void trace(const Thread &t, T0 op0, T1 op1, T2 op2, T3 op3, T4 op4,
             T5 op5, T6 op6, T7 op7, T8 op8, T9 op9, T10 op10)
  {
    printInstructionStart(t);
    printOperand(op0);
    printOperand(op1);
    printOperand(op2);
    printOperand(op3);
    printOperand(op4);
    printOperand(op5);
    printOperand(op6);
    printOperand(op7);
    printOperand(op8);
    printOperand(op9);
    printOperand(op10);
  }

  template<typename T0, typename T1, typename T2, typename T3, typename T4,
  typename T5, typename T6, typename T7, typename T8, typename T9, typename T10,
  typename T11>
  void trace(const Thread &t, T0 op0, T1 op1, T2 op2, T3 op3, T4 op4,
             T5 op5, T6 op6, T7 op7, T8 op8, T9 op9, T10 op10, T11 op11)
  {
    printInstructionStart(t);
    printOperand(op0);
    printOperand(op1);
    printOperand(op2);
    printOperand(op3);
    printOperand(op4);
    printOperand(op5);
    printOperand(op6);
    printOperand(op7);
    printOperand(op8);
    printOperand(op9);
    printOperand(op10);
    printOperand(op11);
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

  static Tracer &get()
  {
    return instance;
  }
};

#endif //_Trace_h_
