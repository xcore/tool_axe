// Copyright (c) 2011, Richard Osborne, All rights reserved
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
#include "SymbolInfo.h"
#include "Register.h"
#include "llvm/Support/raw_ostream.h"
#include <string>
#include <memory>

namespace axe {

class EventableResource;
class SystemState;
class Node;
class Thread;

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &out,
                                     const Register::Reg &r) {
  return out << getRegisterName(r);
}

class Tracer {
private:
  bool tracingEnabled;
  bool useColors;
  llvm::raw_ostream &out;
  uint64_t pos;
  const Thread *thread;
  uint32_t pc;
  bool emittedLineStart;
  SymbolInfo symInfo;

  void green();
  void red();
  void reset();
  void align(unsigned column);

  void printLinePrefix(const Thread &t);
  void printLinePrefix(const Node &n);
  void printLineEnd();
  void printThreadName(const Thread &t);
  void printThreadPC(const Thread &t, uint32_t pc);
  void printInstructionLineStart(const Thread &t, uint32_t pc);

  void printRegWrite(Register::Reg reg, uint32_t value, bool first);

  void printImm(uint32_t op) {
    out << op;
  }

  void printSrcRegister(Register::Reg op);
  void printDestRegister(Register::Reg op);
  void printSrcDestRegister(Register::Reg op);
  void printCPRelOffset(uint32_t op);
  void printDPRelOffset(uint32_t op);

  unsigned parseOperandNum(const char *p, const char *&end);

  void dumpThreadSummary(const Core &core);
  void dumpThreadSummary(const SystemState &system);

  void syscallBegin(const Thread &t);
public:
  Tracer(bool tracing);
  bool getTracingEnabled() const { return tracingEnabled; }
  SymbolInfo *getSymbolInfo() { return &symInfo; }

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
    out << s << "()";
    out.changeColor(llvm::raw_ostream::WHITE);
    printLineEnd();
  }
  template<typename T0>
  void syscall(const Thread &t, const std::string &s,
               T0 op0) {
    syscallBegin(t);
    out << s << '(' << op0 << ')';
    out.changeColor(llvm::raw_ostream::WHITE);
    printLineEnd();
  }
  void timeout(const SystemState &system, ticks_t time);
  void noRunnableThreads(const SystemState &system);
};

} // End axe namespace

#endif //_Trace_h_
