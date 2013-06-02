// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Trace.h"
#include "SystemState.h"
#include "Node.h"
#include "Core.h"
#include "Resource.h"
#include "Exceptions.h"
#include "Instruction.h"
#include "InstructionProperties.h"
#include "InstructionTraceInfo.h"
#include "InstructionOpcode.h"
#include <iomanip>
#include <sstream>
#include <cstring>

using namespace axe;
using namespace Register;

const unsigned mnemonicColumn = 49;
const unsigned regWriteColumn = 87;

void Tracer::setColour(bool enable)
{
  colours = enable ? TerminalColours::ansi : TerminalColours::null;
}

void Tracer::escapeCode(const char *s)
{
  buf << s;
  numEscapeChars += std::strlen(s);
}

void Tracer::printLineEnd()
{
  buf << '\n';
  out << buf.str();
  buf.str("");
  numEscapeChars = 0;
}

void Tracer::printThreadName(const Thread &t)
{
  buf << t.getParent().getCoreName();
  buf << ":t" << t.getNum();
}

void Tracer::printLinePrefix(const Node &n)
{
  green();
  buf << '<';
  buf << 'n' << n.getNodeID();
  buf << '>';
  reset();
}

void Tracer::printLinePrefix(const Thread &t)
{
  // TODO add option to show cycles?
  //buf << std::setw(6) << (uint64_t)thread->time;
  green();
  buf << '<';
  printThreadName(t);
  buf << '>';
  reset();
}

void Tracer::printThreadPC(const Thread &t, uint32_t pc)
{
  const Core *core = &t.getParent();
  const ElfSymbol *sym;
  if (t.getParent().isValidRamAddress(pc) &&
      (sym = symInfo.getFunctionSymbol(core, pc))) {
    buf << sym->name;
    if (sym->value != pc)
      buf << '+' << (pc - sym->value);
    buf << "(0x" << std::hex << pc << std::dec << ')';
  } else {
    buf << "0x" << std::hex << pc << std::dec;
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

void Tracer::printInstructionLineStart(const Thread &t, uint32_t pc)
{
  printLinePrefix(*thread);
  buf << ' ';
  printThreadPC(t, pc);
  buf << ": ";

  // Align
  size_t pos = buf.str().size() - numEscapeChars;
  if (pos < mnemonicColumn) {
    buf << std::setw(mnemonicColumn - pos) << "";
  }

  // Disassemble instruction.
  InstructionOpcode opcode;
  Operands ops;
  instructionDecode(t.getParent(), pc, opcode, ops, true);
  const InstructionProperties &properties =
  instructionProperties[opcode];

  // Special cases.
  // TODO remove this by describing tsetmr as taking an immediate?
  if (opcode == InstructionOpcode::TSETMR_2r) {
    buf << "tsetmr ";
    printDestRegister(getOperandRegister(properties, ops, 0));
    buf << ", ";
    printSrcRegister(getOperandRegister(properties, ops, 1));
    return;
  }
  if (opcode == InstructionOpcode::ADD_2rus &&
      getOperand(properties, ops, 2) == 0) {
    buf << "mov ";
    printDestRegister(getOperandRegister(properties, ops, 0));
    buf << ", ";
    printSrcRegister(getOperandRegister(properties, ops, 1));
    return;
  }

  const char *fmt = instructionTraceInfo[opcode].string;
  for (const char *p = fmt; *p != '\0'; ++p) {
    if (*p != '%') {
      buf << *p;
      continue;
    }
    ++p;
    assert(*p != '\0');
    if (*p == '%') {
      buf << '%';
      continue;
    }
    enum {
      RELATIVE_NONE,
      DP_RELATIVE,
      CP_RELATIVE,
    } relType = RELATIVE_NONE;
    if (std::strncmp(p, "{dp}", 4) == 0) {
      relType = DP_RELATIVE;
      p += 4;
    } else if (std::strncmp(p, "{cp}", 4) == 0) {
      relType = CP_RELATIVE;
      p += 4;
    }
    assert(isdigit(*p));
    char *endp;
    long value = std::strtol(p, &endp, 10);
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

void Tracer::printRegWrite(Register::Reg reg, uint32_t value, bool first)
{
  if (first) {
    buf << ' ';
    // Align
    size_t pos = buf.str().size() - numEscapeChars;
    if (pos < regWriteColumn) {
      buf << std::setw(regWriteColumn - pos) << "";
    }
    buf << "# ";
  } else {
    buf << ", ";
  }
  buf << reg << "=0x" << std::hex << value << std::dec;
}

void Tracer::instructionBegin(const Thread &t)
{
  assert(!thread);
  assert(!emittedLineStart);
  thread = &t;
  pc = t.getRealPc();
}

void Tracer::instructionEnd() {
  assert(thread);
  if (!emittedLineStart) {
    printInstructionLineStart(*thread, pc);
  }
  thread = nullptr;
  emittedLineStart = false;
  printLineEnd();
}

void Tracer::printSrcRegister(Register::Reg reg)
{
  buf << reg << "(0x" << std::hex << thread->regs[reg] << ')'
      << std::dec;
}

void Tracer::printDestRegister(Register::Reg reg)
{
  buf << reg;
}

void Tracer::printSrcDestRegister(Register::Reg reg)
{
  buf << reg << "(0x" << std::hex << thread->regs[reg] << ')'
      << std::dec;
}

void Tracer::printCPRelOffset(uint32_t offset)
{
  uint32_t cpValue = thread->regs[CP];
  uint32_t address = cpValue + (offset << 2);
  const Core *core = &thread->getParent();
  const ElfSymbol *sym, *cpSym;
  if ((sym = symInfo.getDataSymbol(core, address)) &&
      sym->value == address &&
      (cpSym = symInfo.getGlobalSymbol(core, "_cp")) &&
      cpSym->value == cpValue) {
    buf << sym->name;
    buf << "(0x" << std::hex << address << ')';
  } else {
    buf << offset;
  }
}

void Tracer::printDPRelOffset(uint32_t offset)
{
  uint32_t dpValue = thread->regs[DP];
  uint32_t address = dpValue + (offset << 2);
  const Core *core = &thread->getParent();
  const ElfSymbol *sym, *dpSym;
  if ((sym = symInfo.getDataSymbol(core, address)) &&
      sym->value == address &&
      (dpSym = symInfo.getGlobalSymbol(core, "_dp")) &&
      dpSym->value == dpValue) {
    buf << sym->name;
    buf << "(0x" << std::hex << address << std::dec << ')';
  } else {
    buf << offset;
  }
}

void Tracer::regWrite(Reg reg, uint32_t value)
{
  assert(thread);
  bool first = !emittedLineStart;
  if (!emittedLineStart) {
    printInstructionLineStart(*thread, pc);
  }
  printRegWrite(reg, value, first);
  emittedLineStart = true;
}

void Tracer::SSwitchRead(const Node &node, uint32_t retAddress, uint16_t regNum)
{
  assert(!emittedLineStart);
  printLinePrefix(node);
  red();
  buf << " SSwitch read: ";
  buf << "register 0x" << std::hex << regNum;
  buf << ", reply address 0x" << retAddress << std::dec;
  reset();
  printLineEnd();
}

void Tracer::
SSwitchWrite(const Node &node, uint32_t retAddress, uint16_t regNum,
             uint32_t value)
{
  assert(!emittedLineStart);
  printLinePrefix(node);
  red();
  buf << " SSwitch write: ";
  buf << "register 0x" << std::hex << regNum;
  buf << ", value 0x" << value;
  buf << ", reply address 0x" << retAddress << std::dec;
  reset();
  printLineEnd();
}

void Tracer::SSwitchNack(const Node &node, uint32_t dest)
{
  assert(!emittedLineStart);
  printLinePrefix(node);
  red();
  buf << " SSwitch reply: NACK";
  buf << ", destintion 0x" << std::hex << dest << std::dec;
  reset();
  printLineEnd();
}

void Tracer::SSwitchAck(const Node &node, uint32_t dest)
{
  assert(!emittedLineStart);
  printLinePrefix(node);
  red();
  buf << " SSwitch reply: ACK";
  buf << ", destintion 0x" << std::hex << dest << std::dec;
  reset();
  printLineEnd();
}

void Tracer::SSwitchAck(const Node &node, uint32_t data, uint32_t dest)
{
  assert(!emittedLineStart);
  printLinePrefix(node);
  red();
  buf << " SSwitch reply: ACK";
  buf << ", data 0x" << std::hex << data;
  buf << ", destintion 0x" << dest << std::dec;
  reset();
  printLineEnd();
}

void Tracer::
event(const Thread &t, const EventableResource &res, uint32_t pc,
      uint32_t ev)
{
  assert(!emittedLineStart);
  printThreadName(t);
  red();
  buf << " Event caused by "
       << Resource::getResourceName(static_cast<const Resource&>(res).getType())
       << " 0x" << std::hex << (uint32_t)res.getID() << std::dec;
  reset();
  regWrite(ED, ev);
  printLineEnd();
}

void Tracer::
interrupt(const Thread &t, const EventableResource &res, uint32_t pc,
          uint32_t ssr, uint32_t spc, uint32_t sed, uint32_t ed)
{
  assert(!emittedLineStart);
  printThreadName(t);
  red();
  buf << " Interrupt caused by "
       << Resource::getResourceName(static_cast<const Resource&>(res).getType())
       << " 0x" << std::hex << (uint32_t)res.getID() << std::dec;
  reset();
  printRegWrite(ED, ed, true);
  printRegWrite(SSR, ssr, false);
  printRegWrite(SPC, spc, false);
  printRegWrite(SED, sed, false);
  printLineEnd();
}

void Tracer::
exception(const Thread &t, uint32_t et, uint32_t ed,
          uint32_t sed, uint32_t ssr, uint32_t spc)
{
  assert(!emittedLineStart);
  printInstructionLineStart(*thread, pc);
  printLineEnd();
  printThreadName(t);
  red();
  buf << ' ' << Exceptions::getExceptionName(et) << " exception";
  reset();
  printRegWrite(ET, et, true);
  printRegWrite(ED, ed, false);
  printRegWrite(SSR, ssr, false);
  printRegWrite(SPC, spc, false);
  printRegWrite(SED, sed, false);
  emittedLineStart = true;
}

void Tracer::
syscallBegin(const Thread &t)
{
  assert(!emittedLineStart);
  printLinePrefix(t);
  red();
  buf << " Syscall ";
}

void Tracer::dumpThreadSummary(const Core &core)
{
  for (unsigned i = 0; i < NUM_THREADS; i++) {
    const Thread &t = core.getThread(i);
    if (!t.isInUse())
      continue;
    buf << "Thread ";
    printThreadName(t);
    if (t.waiting()) {
      if (Resource *res = t.pausedOn) {
        buf << " paused on ";
        buf << Resource::getResourceName(res->getType());
        buf << " 0x" << std::hex << res->getID();
      } else if (t.eeble()) {
        buf << " waiting for events";
        if (t.ieble())
          buf << " or interrupts";
      } else if (t.ieble()) {
        buf << " waiting for interrupts";
      } else {
        buf << " paused";
      }
    }
    buf << " at ";
    printThreadPC(t, t.getRealPc());
    printLineEnd();
  }
}

void Tracer::dumpThreadSummary(const SystemState &system)
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

void Tracer::timeout(const SystemState &system, ticks_t time)
{
  assert(!emittedLineStart);
  red();
  buf << "Timeout after " << time << " cycles";
  reset();
  printLineEnd();
  dumpThreadSummary(system);
}

void Tracer::noRunnableThreads(const SystemState &system)
{
  assert(!emittedLineStart);
  red();
  buf << "No more runnable threads";
  reset();
  printLineEnd();
  dumpThreadSummary(system);
}

