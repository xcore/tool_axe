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
#include <iomanip>
#include <sstream>
#include <cstring>

using namespace Register;

const unsigned mnemonicColumn = 49;
const unsigned regWriteColumn = 87;

Tracer Tracer::instance;

Tracer::PushLineState::PushLineState() :
  needRestore(false),
  line(Tracer::get().line.pending)
{
  if (Tracer::get().line.thread) {
    std::swap(Tracer::get().line, line);
    needRestore = true;
  }
}

Tracer::PushLineState::~PushLineState()
{
  if (needRestore) {
    std::swap(Tracer::get().line, line);
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
  unsigned pc = line.thread->fromPc(line.thread->pc);
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
}

void Tracer::printOperand(SrcRegister op)
{
  Register::Reg reg = op.getRegister();
  *line.buf << reg << "(0x" << std::hex << line.thread->regs[reg] << ')'
       << std::dec;
}

void Tracer::printOperand(DestRegister op)
{
  *line.buf << op.getRegister();
}

void Tracer::printOperand(SrcDestRegister op)
{
  Register::Reg reg = op.getRegister();
  *line.buf << reg << "(0x" << std::hex << line.thread->regs[reg] << ')'
       << std::dec;
}

void Tracer::printOperand(CPRelOffset op)
{
  uint32_t cpValue = line.thread->regs[CP];
  uint32_t address = cpValue + (op.getOffset() << 2);
  const Core *core = &line.thread->getParent();
  const ElfSymbol *sym, *cpSym;
  if ((sym = symInfo.getDataSymbol(core, address)) &&
      sym->value == address &&
      (cpSym = symInfo.getGlobalSymbol(core, "_cp")) &&
      cpSym->value == cpValue) {
    *line.buf << sym->name;
    *line.buf << "(0x" << std::hex << address << ')';
  } else {
    *line.buf << op.getOffset();
  }
}

void Tracer::printOperand(DPRelOffset op)
{
  uint32_t dpValue = line.thread->regs[DP];
  uint32_t address = dpValue + (op.getOffset() << 2);
  const Core *core = &line.thread->getParent();
  const ElfSymbol *sym, *dpSym;
  if ((sym = symInfo.getDataSymbol(core, address)) &&
      sym->value == address &&
      (dpSym = symInfo.getGlobalSymbol(core, "_dp")) &&
      dpSym->value == dpValue) {
    *line.buf << sym->name;
    *line.buf << "(0x" << std::hex << address << std::dec << ')';
  } else {
    *line.buf << op.getOffset();
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
  PushLineState save;
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
  PushLineState save;
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
  PushLineState save;
  printCommonStart(node);
  red();
  *line.buf << " SSwitch reply: NACK";
  *line.buf << ", destintion 0x" << std::hex << dest << std::dec;
  reset();
  printCommonEnd();
}

void Tracer::SSwitchAck(const Node &node, uint32_t dest)
{
  PushLineState save;
  printCommonStart(node);
  red();
  *line.buf << " SSwitch reply: ACK";
  *line.buf << ", destintion 0x" << std::hex << dest << std::dec;
  reset();
  printCommonEnd();
}

void Tracer::SSwitchAck(const Node &node, uint32_t data, uint32_t dest)
{
  PushLineState save;
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
  PushLineState save;
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
  PushLineState save;
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
  PushLineState save;
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

void Tracer::noRunnableThreads(const SystemState &system)
{
  printCommonStart();
  red();
  *line.buf << "No more runnable threads";
  reset();
  printCommonEnd();
  dumpThreadSummary(system);
}

