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

Tracer::PushLineState::PushLineState(Tracer &p) :
  needRestore(false),
  line(p.line.pending),
  parent(p)
{
  if (parent.line.thread) {
    std::swap(parent.line, line);
    needRestore = true;
  }
}

Tracer::PushLineState::~PushLineState()
{
  if (needRestore) {
    std::swap(parent.line, line);
  }
}

void Tracer::setColour(bool enable)
{
  colours = enable ? TerminalColours::ansi : TerminalColours::null;
}

void Tracer::escapeCode(const char *s)
{
  *line.buf << s;
  line.numEscapeChars += std::strlen(s);
}

void Tracer::printCommonEnd()
{
  *line.buf << '\n';
  if (line.out) {
    *line.out << line.buf->str() << line.pending->str();
    line.buf->str("");
  }
  line.thread = 0;
}

void Tracer::printCommonStart()
{
  if (line.pending) {
    line.pending->str("");
  }
  line.numEscapeChars = 0;
  line.hadRegWrite = false;
  line.thread = 0;
}

void Tracer::printThreadName()
{
  *line.buf << line.thread->getParent().getCoreName();
  *line.buf << ":t" << line.thread->getNum();
}

void Tracer::printCommonStart(const Thread &t)
{
  printCommonStart();
  line.thread = &t;

  // TODO add option to show cycles?
  //*line.buf << std::setw(6) << (uint64_t)line.thread->time;
  green();
  *line.buf << '<';
  printThreadName();
  *line.buf << '>';
  reset();
}

void Tracer::printCommonStart(const Node &n)
{
  printCommonStart();

  green();
  *line.buf << '<';
  *line.buf << 'n' << n.getNodeID();
  *line.buf << '>';
  reset();
}

void Tracer::printThreadPC()
{
  unsigned pc = line.thread->getRealPc();
  const Core *core = &line.thread->getParent();
  const ElfSymbol *sym;
  if (line.thread->isInRam() &&
      (sym = symInfo.getFunctionSymbol(core, pc))) {
    *line.buf << sym->name;
    if (sym->value != pc)
      *line.buf << '+' << (pc - sym->value);
    *line.buf << "(0x" << std::hex << pc << std::dec << ')';
  } else {
    *line.buf << "0x" << std::hex << pc << std::dec;
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

Register::Reg
getOperandRegister(const InstructionProperties &properties,
                   const Operands &ops, unsigned i)
{
  if (i >= properties.getNumExplicitOperands())
    return properties.getImplicitOperand(i - properties.getNumExplicitOperands());
  if (properties.getNumExplicitOperands() > 3)
    return static_cast<Register::Reg>(ops.lops[i]);
  return static_cast<Register::Reg>(ops.ops[i]);
}

void Tracer::printInstructionStart(const Thread &t)
{
  printCommonStart(t);
  *line.buf << ' ';
  printThreadPC();
  *line.buf << ": ";

  // Align
  size_t pos = line.buf->str().size() - line.numEscapeChars;
  if (pos < mnemonicColumn) {
    *line.buf << std::setw(mnemonicColumn - pos) << "";
  }

  // Disassemble instruction.
  InstructionOpcode opcode;
  Operands ops;
  instructionDecode(t.getParent(), t.getRealPc(), opcode, ops, true);
  const InstructionProperties &properties =
    instructionProperties[opcode];

  // Special cases.
  // TODO remove this by describing tsetmr as taking an immediate?
  if (opcode == InstructionOpcode::TSETMR_2r) {
    *line.buf << "tsetmr ";
    printDestRegister(getOperandRegister(properties, ops, 0));
    *line.buf << ", ";
    printSrcRegister(getOperandRegister(properties, ops, 1));
    return;
  }
  if (opcode == InstructionOpcode::ADD_2rus &&
      getOperand(properties, ops, 2) == 0) {
    *line.buf << "mov ";
    printDestRegister(getOperandRegister(properties, ops, 0));
    *line.buf << ", ";
    printSrcRegister(getOperandRegister(properties, ops, 1));
    return;
  }

  const char *fmt = instructionTraceInfo[opcode].string;
  for (const char *p = fmt; *p != '\0'; ++p) {
    if (*p != '%') {
      *line.buf << *p;
      continue;
    }
    ++p;
    assert(*p != '\0');
    if (*p == '%') {
      *line.buf << '%';
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

void Tracer::printSrcRegister(Register::Reg reg)
{
  *line.buf << reg << "(0x" << std::hex << line.thread->regs[reg] << ')'
       << std::dec;
}

void Tracer::printDestRegister(Register::Reg reg)
{
  *line.buf << reg;
}

void Tracer::printSrcDestRegister(Register::Reg reg)
{
  *line.buf << reg << "(0x" << std::hex << line.thread->regs[reg] << ')'
       << std::dec;
}

void Tracer::printCPRelOffset(uint32_t offset)
{
  uint32_t cpValue = line.thread->regs[CP];
  uint32_t address = cpValue + (offset << 2);
  const Core *core = &line.thread->getParent();
  const ElfSymbol *sym, *cpSym;
  if ((sym = symInfo.getDataSymbol(core, address)) &&
      sym->value == address &&
      (cpSym = symInfo.getGlobalSymbol(core, "_cp")) &&
      cpSym->value == cpValue) {
    *line.buf << sym->name;
    *line.buf << "(0x" << std::hex << address << ')';
  } else {
    *line.buf << offset;
  }
}

void Tracer::printDPRelOffset(uint32_t offset)
{
  uint32_t dpValue = line.thread->regs[DP];
  uint32_t address = dpValue + (offset << 2);
  const Core *core = &line.thread->getParent();
  const ElfSymbol *sym, *dpSym;
  if ((sym = symInfo.getDataSymbol(core, address)) &&
      sym->value == address &&
      (dpSym = symInfo.getGlobalSymbol(core, "_dp")) &&
      dpSym->value == dpValue) {
    *line.buf << sym->name;
    *line.buf << "(0x" << std::hex << address << std::dec << ')';
  } else {
    *line.buf << offset;
  }
}

void Tracer::regWrite(Reg reg, uint32_t value)
{
  if (!line.hadRegWrite) {
    *line.buf << ' ';
    // Align
    size_t pos = line.buf->str().size() - line.numEscapeChars;
    if (pos < regWriteColumn) {
      *line.buf << std::setw(regWriteColumn - pos) << "";
    }
    *line.buf << "# ";
  } else {
    *line.buf << ", ";
  }
  *line.buf << reg << "=0x" << std::hex << value << std::dec;
  line.hadRegWrite = true;
}

void Tracer::SSwitchRead(const Node &node, uint32_t retAddress, uint16_t regNum)
{
  PushLineState save(*this);
  printCommonStart(node);
  red();
  *line.buf << " SSwitch read: ";
  *line.buf << "register 0x" << std::hex << regNum;
  *line.buf << ", reply address 0x" << retAddress << std::dec;
  reset();
  printCommonEnd();
}

void Tracer::
SSwitchWrite(const Node &node, uint32_t retAddress, uint16_t regNum,
             uint32_t value)
{
  PushLineState save(*this);
  printCommonStart(node);
  red();
  *line.buf << " SSwitch write: ";
  *line.buf << "register 0x" << std::hex << regNum;
  *line.buf << ", value 0x" << value;
  *line.buf << ", reply address 0x" << retAddress << std::dec;
  reset();
  printCommonEnd();
}

void Tracer::SSwitchNack(const Node &node, uint32_t dest)
{
  PushLineState save(*this);
  printCommonStart(node);
  red();
  *line.buf << " SSwitch reply: NACK";
  *line.buf << ", destintion 0x" << std::hex << dest << std::dec;
  reset();
  printCommonEnd();
}

void Tracer::SSwitchAck(const Node &node, uint32_t dest)
{
  PushLineState save(*this);
  printCommonStart(node);
  red();
  *line.buf << " SSwitch reply: ACK";
  *line.buf << ", destintion 0x" << std::hex << dest << std::dec;
  reset();
  printCommonEnd();
}

void Tracer::SSwitchAck(const Node &node, uint32_t data, uint32_t dest)
{
  PushLineState save(*this);
  printCommonStart(node);
  red();
  *line.buf << " SSwitch reply: ACK";
  *line.buf << ", data 0x" << std::hex << data;
  *line.buf << ", destintion 0x" << dest << std::dec;
  reset();
  printCommonEnd();
}

void Tracer::
event(const Thread &t, const EventableResource &res, uint32_t pc,
      uint32_t ev)
{
  PushLineState save(*this);
  printCommonStart(t);
  red();
  *line.buf << " Event caused by "
       << Resource::getResourceName(static_cast<const Resource&>(res).getType())
       << " 0x" << std::hex << (uint32_t)res.getID() << std::dec;
  reset();
  regWrite(ED, ev);
  printCommonEnd();
}

void Tracer::
interrupt(const Thread &t, const EventableResource &res, uint32_t pc,
          uint32_t ssr, uint32_t spc, uint32_t sed, uint32_t ed)
{
  PushLineState save(*this);
  printCommonStart(t);
  red();
  *line.buf << " Interrupt caused by "
       << Resource::getResourceName(static_cast<const Resource&>(res).getType())
       << " 0x" << std::hex << (uint32_t)res.getID() << std::dec;
  reset();
  regWrite(ED, ed);
  regWrite(SSR, ssr);
  regWrite(SPC, spc);
  regWrite(SED, sed);
  printCommonEnd();
}

void Tracer::
exception(const Thread &t, uint32_t et, uint32_t ed,
          uint32_t sed, uint32_t ssr, uint32_t spc)
{
  PushLineState save(*this);
  printCommonStart(t);
  red();
  *line.buf << ' ' << Exceptions::getExceptionName(et) << " exception";
  reset();
  regWrite(ET, et);
  regWrite(ED, ed);
  regWrite(SSR, ssr);
  regWrite(SPC, spc);
  regWrite(SED, sed);
  printCommonEnd();
}

void Tracer::
syscallBegin(const Thread &t)
{
  printCommonStart(t);
  red();
  *line.buf << " Syscall ";
}

void Tracer::dumpThreadSummary(const Core &core)
{
  for (unsigned i = 0; i < NUM_THREADS; i++) {
    const Thread &t = core.getThread(i);
    if (!t.isInUse())
      continue;
    const Thread &ts = t;
    printCommonStart();
    line.thread = &ts;
    *line.buf << "Thread ";
    printThreadName();
    if (ts.waiting()) {
      if (Resource *res = ts.pausedOn) {
        *line.buf << " paused on ";
        *line.buf << Resource::getResourceName(res->getType());
        *line.buf << " 0x" << std::hex << res->getID();
      } else if (ts.eeble()) {
        *line.buf << " waiting for events";
        if (ts.ieble())
          *line.buf << " or interrupts";
      } else if (ts.ieble()) {
        *line.buf << " waiting for interrupts";
      } else {
        *line.buf << " paused";
      }
    }
    *line.buf << " at ";
    printThreadPC();
    printCommonEnd();
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
  printCommonStart();
  red();
  *line.buf << "Timeout after " << time << " cycles";
  reset();
  printCommonEnd();
  dumpThreadSummary(system);
}

void Tracer::noRunnableThreads(const SystemState &system)
{
  printCommonStart();
  red();
  *line.buf << "No more runnable threads";
  reset();
  printCommonEnd();
  dumpThreadSummary(system);
}

