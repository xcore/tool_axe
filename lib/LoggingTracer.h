// Copyright (c) 2011-2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _LoggingTracer_h
#define _LoggingTracer_h

#include "Tracer.h"
#include "Config.h"
#include "SymbolInfo.h"
#include "Register.h"
#include <string>
#include <memory>

namespace llvm {
  class raw_ostream;
};

namespace axe {
  class EventableResource;
  class SystemState;
  class Node;
  class Thread;
  
  class LoggingTracer : public Tracer {
  private:
    bool traceCycles;
    bool useColors;
    llvm::raw_ostream &out;
    uint64_t pos;
    const Thread *thread;
    uint32_t pc;
    bool emittedLineStart;
    const SymbolInfo *symInfo;

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

    void printImm(uint32_t op);

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
    LoggingTracer(bool traceCycles);

    void attach(const SystemState &systemState) override;

    void instructionBegin(const Thread &t) override;
    
    void regWrite(Register::Reg reg, uint32_t value) override;
    
    void instructionEnd() override;
    
    void SSwitchRead(const Node &node, uint32_t retAddress,
                     uint16_t regNum) override;
    void SSwitchWrite(const Node &node, uint32_t retAddress,
                      uint16_t regNum, uint32_t value) override;
    void SSwitchNack(const Node &node, uint32_t dest) override;
    void SSwitchAck(const Node &node, uint32_t dest) override;
    void SSwitchAck(const Node &node, uint32_t data, uint32_t dest) override;
    
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
    void timeout(const SystemState &system, ticks_t time) override;
    void noRunnableThreads(const SystemState &system) override;
  };
  
} // End axe namespace

#endif // _LoggingTracer_h
