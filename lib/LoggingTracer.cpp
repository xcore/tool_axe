// Copyright (c) 2011-2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "LoggingTracer.h"
#include "SystemState.h"
#include "Node.h"
#include "Core.h"
#include "Resource.h"
#include "Exceptions.h"
#include "Instruction.h"
#include "InstructionProperties.h"
#include "InstructionTraceInfo.h"
#include "InstructionOpcode.h"

#include "llvm/Support/raw_ostream.h"
#include <iomanip>
#include <sstream>
#include <cstring>

using namespace axe;
using namespace Register;

const unsigned mnemonicColumn = 49;
const unsigned regWriteColumn = 87;

static llvm::raw_ostream &operator<<(llvm::raw_ostream &out,
                                     const Register::Reg &r) {
  return out << getRegisterName(r);
}

LoggingTracer::LoggingTracer() :
  useColors(llvm::outs().has_colors()),
  out(llvm::outs()),
  pos(out.tell()),
  thread(nullptr),
  emittedLineStart(false),
  symInfo(0)
{
}

void LoggingTracer::attach(const SystemState &systemState)
{
  symInfo = &systemState.getSymbolInfo();
}

void LoggingTracer::green()
{
  if (useColors)
    out.changeColor(llvm::raw_ostream::GREEN);
}

void LoggingTracer::red()
{
  if (useColors)
    out.changeColor(llvm::raw_ostream::RED);
}

void LoggingTracer::reset()
{
  if (useColors)
    out.resetColor();
}

void LoggingTracer::align(unsigned column)
{
  uint64_t currentPos = out.tell() - pos;
  if (currentPos >= column) {
    out << ' ';
    return;
  }
  unsigned numSpaces = column - currentPos;
  static const char spaces[] =
  "                                                  ";
  const int arraySize = sizeof(spaces) - 1;
  while (numSpaces >= arraySize) {
    out.write(spaces, arraySize);
    numSpaces -= arraySize;
  }
  if (numSpaces)
    out.write(spaces, numSpaces);
}

void LoggingTracer::printLineEnd()
{
  out << '\n';
  pos = out.tell();
}

void LoggingTracer::printThreadName(const Thread &t)
{
  out << t.getParent().getCoreName();
  out << ":t" << t.getNum();
}

void LoggingTracer::printLinePrefix(const Node &n)
{
  green();
  out << '<';
  out << 'n' << n.getNodeID();
  out << '>';
  reset();
}

void LoggingTracer::printLinePrefix(const Thread &t)
{
  // TODO add option to show cycles?
  //out << std::setw(6) << (uint64_t)thread->time;
  green();
  out << '<';
  printThreadName(t);
  out << '>';
  reset();
}

void LoggingTracer::printThreadPC(const Thread &t, uint32_t pc)
{
  const Core *core = &t.getParent();
  const ElfSymbol *sym;
  if (t.getParent().isValidRamAddress(pc) &&
      (sym = symInfo->getFunctionSymbol(core, pc))) {
    out << sym->name;
    if (sym->value != pc)
      out << '+' << (pc - sym->value);
    out << "(0x";
    out.write_hex(pc);
    out << ')';
  } else {
    out << "0x";
    out.write_hex(pc);
  }
}

static uint32_t getOperand(const InstructionProperties &properties,
                           const Operands &operands, unsigned i)
{
  if (properties.getNumExplicitOperands() > 3) {
    return operands.lops[i];
  }
  return operands.ops[i];
}

static Register::Reg
getOperandRegister(const InstructionProperties &properties,
                   const Operands &ops, unsigned i)
{
  if (i >= properties.getNumExplicitOperands())
    return properties.getImplicitOperand(i - properties.getNumExplicitOperands());
  if (properties.getNumExplicitOperands() > 3)
    return static_cast<Register::Reg>(ops.lops[i]);
  return static_cast<Register::Reg>(ops.ops[i]);
}

unsigned LoggingTracer::parseOperandNum(const char *p, const char *&end)
{
  // Operands are currently restricted to one digit.
  assert(isdigit(*p) && !isdigit(*(p + 1)));
  end = p + 1;
  return *p - '0';
}

void LoggingTracer::printInstructionLineStart(const Thread &t, uint32_t pc)
{
  printLinePrefix(*thread);
  out << ' ';
  printThreadPC(t, pc);
  out << ":";
  
  // Align
  align(mnemonicColumn);
  
  // Disassemble instruction.
  InstructionOpcode opcode;
  Operands ops;
  instructionDecode(t.getParent(), pc, opcode, ops, true);
  const InstructionProperties &properties =
  instructionProperties[opcode];
  
  // Special cases.
  // TODO remove this by describing tsetmr as taking an immediate?
  if (opcode == InstructionOpcode::TSETMR_2r) {
    out << "tsetmr ";
    printDestRegister(getOperandRegister(properties, ops, 0));
    out << ", ";
    printSrcRegister(getOperandRegister(properties, ops, 1));
    return;
  }
  if (opcode == InstructionOpcode::ADD_2rus &&
      getOperand(properties, ops, 2) == 0) {
    out << "mov ";
    printDestRegister(getOperandRegister(properties, ops, 0));
    out << ", ";
    printSrcRegister(getOperandRegister(properties, ops, 1));
    return;
  }
  
  const char *fmt = instructionTraceInfo[opcode].string;
  for (const char *p = fmt; *p != '\0'; ++p) {
    if (*p != '%') {
      out << *p;
      continue;
    }
    ++p;
    assert(*p != '\0');
    if (*p == '%') {
      out << '%';
      continue;
    }
    enum {
      RELATIVE_NONE,
      DP_RELATIVE,
      CP_RELATIVE,
    } relType = RELATIVE_NONE;
    if (*p == '{') {
      if (*(p + 1) == 'd') {
        assert(std::strncmp(p, "{dp}", 4) == 0);
        relType = DP_RELATIVE;
        p += 4;
      } else {
        assert(std::strncmp(p, "{cp}", 4) == 0);
        relType = CP_RELATIVE;
        p += 4;
      }
    }
    const char *endp;
    unsigned value = parseOperandNum(p, endp);
    p = endp - 1;
    assert(value >= 0 && value < properties.getNumOperands());
    switch (properties.getOperandType(value)) {
    default: assert(0 && "Unexpected operand type");
    case OperandProperties::out:
      printDestRegister(getOperandRegister(properties, ops, value));
      break;
    case OperandProperties::in:
      printSrcRegister(getOperandRegister(properties, ops, value));
      break;
    case OperandProperties::inout:
      printSrcDestRegister(getOperandRegister(properties, ops, value));
      break;
    case OperandProperties::imm:
      switch (relType) {
      case RELATIVE_NONE:
        printImm(getOperand(properties, ops, value));
        break;
      case CP_RELATIVE:
        printCPRelOffset(getOperand(properties, ops, value));
        break;
      case DP_RELATIVE:
        printDPRelOffset(getOperand(properties, ops, value));
        break;
      }
      break;
    }
  }
}

void LoggingTracer::printRegWrite(Register::Reg reg, uint32_t value, bool first)
{
  if (first) {
    align(regWriteColumn);
    out << "# ";
  } else {
    out << ", ";
  }
  out << reg << "=0x";
  out.write_hex(value);
}

void LoggingTracer::printImm(uint32_t op) {
  out << op;
}

void LoggingTracer::instructionBegin(const Thread &t)
{
  assert(!thread);
  assert(!emittedLineStart);
  thread = &t;
  pc = t.getRealPc();
}

void LoggingTracer::instructionEnd() {
  assert(thread);
  if (!emittedLineStart) {
    printInstructionLineStart(*thread, pc);
  }
  thread = nullptr;
  emittedLineStart = false;
  printLineEnd();
}

void LoggingTracer::printSrcRegister(Register::Reg reg)
{
  out << reg << "(0x";
  out.write_hex(thread->regs[reg]);
  out << ')';
}

void LoggingTracer::printDestRegister(Register::Reg reg)
{
  out << reg;
}

void LoggingTracer::printSrcDestRegister(Register::Reg reg)
{
  out << reg << "(0x";
  out.write_hex(thread->regs[reg]);
  out << ')';
}

void LoggingTracer::printCPRelOffset(uint32_t offset)
{
  uint32_t cpValue = thread->regs[CP];
  uint32_t address = cpValue + (offset << 2);
  const Core *core = &thread->getParent();
  const ElfSymbol *sym, *cpSym;
  if ((sym = symInfo->getDataSymbol(core, address)) &&
      sym->value == address &&
      (cpSym = symInfo->getGlobalSymbol(core, "_cp")) &&
      cpSym->value == cpValue) {
    out << sym->name << "(0x";
    out.write_hex(address);
    out << ')';
  } else {
    out << offset;
  }
}

void LoggingTracer::printDPRelOffset(uint32_t offset)
{
  uint32_t dpValue = thread->regs[DP];
  uint32_t address = dpValue + (offset << 2);
  const Core *core = &thread->getParent();
  const ElfSymbol *sym, *dpSym;
  if ((sym = symInfo->getDataSymbol(core, address)) &&
      sym->value == address &&
      (dpSym = symInfo->getGlobalSymbol(core, "_dp")) &&
      dpSym->value == dpValue) {
    out << sym->name << "(0x";
    out.write_hex(address);
    out << ')';
  } else {
    out << offset;
  }
}

void LoggingTracer::regWrite(Reg reg, uint32_t value)
{
  assert(thread);
  bool first = !emittedLineStart;
  if (!emittedLineStart) {
    printInstructionLineStart(*thread, pc);
  }
  printRegWrite(reg, value, first);
  emittedLineStart = true;
}

void LoggingTracer::SSwitchRead(const Node &node, uint32_t retAddress,
                                uint16_t regNum)
{
  assert(!emittedLineStart);
  printLinePrefix(node);
  red();
  out << " SSwitch read: ";
  out << "register 0x";
  out.write_hex(regNum);
  out << ", reply address 0x";
  out.write_hex(retAddress);
  reset();
  printLineEnd();
}

void LoggingTracer::
SSwitchWrite(const Node &node, uint32_t retAddress, uint16_t regNum,
             uint32_t value)
{
  assert(!emittedLineStart);
  printLinePrefix(node);
  red();
  out << " SSwitch write: ";
  out << "register 0x";
  out.write_hex(regNum);
  out << ", value 0x";
  out.write_hex(value);
  out << ", reply address 0x";
  out.write_hex(retAddress);
  reset();
  printLineEnd();
}

void LoggingTracer::SSwitchNack(const Node &node, uint32_t dest)
{
  assert(!emittedLineStart);
  printLinePrefix(node);
  red();
  out << " SSwitch reply: NACK";
  out << ", destintion 0x";
  out.write_hex(dest);
  reset();
  printLineEnd();
}

void LoggingTracer::SSwitchAck(const Node &node, uint32_t dest)
{
  assert(!emittedLineStart);
  printLinePrefix(node);
  red();
  out << " SSwitch reply: ACK";
  out << ", destintion 0x";
  out.write_hex(dest);
  reset();
  printLineEnd();
}

void LoggingTracer::SSwitchAck(const Node &node, uint32_t data, uint32_t dest)
{
  assert(!emittedLineStart);
  printLinePrefix(node);
  red();
  out << " SSwitch reply: ACK";
  out << ", data 0x";
  out.write_hex(data);
  out << ", destintion 0x";
  out.write_hex(dest);
  reset();
  printLineEnd();
}

void LoggingTracer::
event(const Thread &t, const EventableResource &res, uint32_t pc,
      uint32_t ev)
{
  assert(!emittedLineStart);
  printThreadName(t);
  red();
  out << " Event caused by ";
  out << Resource::getResourceName(static_cast<const Resource&>(res).getType());
  out << " 0x";
  out.write_hex((uint32_t)res.getID());
  reset();
  regWrite(ED, ev);
  printLineEnd();
}

void LoggingTracer::
interrupt(const Thread &t, const EventableResource &res, uint32_t pc,
          uint32_t ssr, uint32_t spc, uint32_t sed, uint32_t ed)
{
  assert(!emittedLineStart);
  printThreadName(t);
  red();
  out << " Interrupt caused by ";
  out << Resource::getResourceName(static_cast<const Resource&>(res).getType());
  out << " 0x";
  out.write_hex((uint32_t)res.getID());
  reset();
  printRegWrite(ED, ed, true);
  printRegWrite(SSR, ssr, false);
  printRegWrite(SPC, spc, false);
  printRegWrite(SED, sed, false);
  printLineEnd();
}

void LoggingTracer::
exception(const Thread &t, uint32_t et, uint32_t ed,
          uint32_t sed, uint32_t ssr, uint32_t spc)
{
  assert(!emittedLineStart);
  printInstructionLineStart(*thread, pc);
  printLineEnd();
  printThreadName(t);
  red();
  out << ' ' << Exceptions::getExceptionName(et) << " exception";
  reset();
  printRegWrite(ET, et, true);
  printRegWrite(ED, ed, false);
  printRegWrite(SSR, ssr, false);
  printRegWrite(SPC, spc, false);
  printRegWrite(SED, sed, false);
  emittedLineStart = true;
}

void LoggingTracer::
syscallBegin(const Thread &t)
{
  assert(!emittedLineStart);
  printLinePrefix(t);
  red();
  out << " Syscall ";
}

void LoggingTracer::syscall(const Thread &t, const std::string &s) {
  syscallBegin(t);
  out << s << "()";
  out.changeColor(llvm::raw_ostream::WHITE);
  printLineEnd();
}

void LoggingTracer::syscall(const Thread &t, const std::string &s,
                     uint32_t op0) {
  syscallBegin(t);
  out << s << '(' << op0 << ')';
  out.changeColor(llvm::raw_ostream::WHITE);
  printLineEnd();
}

void LoggingTracer::dumpThreadSummary(const Core &core)
{
  for (unsigned i = 0; i < NUM_THREADS; i++) {
    const Thread &t = core.getThread(i);
    if (!t.isInUse())
      continue;
    out << "Thread ";
    printThreadName(t);
    if (t.waiting()) {
      if (Resource *res = t.pausedOn) {
        out << " paused on ";
        out << Resource::getResourceName(res->getType());
        out << " 0x";
        out.write_hex(res->getID());
      } else if (t.eeble()) {
        out << " waiting for events";
        if (t.ieble())
          out << " or interrupts";
      } else if (t.ieble()) {
        out << " waiting for interrupts";
      } else {
        out << " paused";
      }
    }
    out << " at ";
    printThreadPC(t, t.getRealPc());
    printLineEnd();
  }
}

void LoggingTracer::dumpThreadSummary(const SystemState &system)
{
  for (auto outerIt = system.node_begin(), outerE = system.node_end();
       outerIt != outerE; ++outerIt) {
    const Node &node = **outerIt;
    for (auto innerIt = node.core_begin(), innerE = node.core_end();
         innerIt != innerE; ++innerIt) {
      dumpThreadSummary(**innerIt);
    }
  }
}

void LoggingTracer::timeout(const SystemState &system, ticks_t time)
{
  assert(!emittedLineStart);
  red();
  out << "Timeout after " << time << " cycles";
  reset();
  printLineEnd();
  dumpThreadSummary(system);
}

void LoggingTracer::noRunnableThreads(const SystemState &system)
{
  assert(!emittedLineStart);
  red();
  out << "No more runnable threads";
  reset();
  printLineEnd();
  dumpThreadSummary(system);
}
