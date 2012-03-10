// Copyright (c) 2011-12, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Thread.h"
#include "Core.h"
#include "Node.h"
#include "SystemState.h"
#include "Trace.h"
#include "Exceptions.h"
#include "BitManip.h"
#include "SyscallHandler.h"
#include "JIT.h"
#include "CRC.h"
#include "InstructionHelpers.h"
#include "Synchroniser.h"
#include "Chanend.h"
#include "InstructionProperties.h"
#include <iostream>

const char *registerNames[] = {
  "r0",
  "r1",
  "r2",
  "r3",
  "r4",
  "r5",
  "r6",
  "r7",
  "r8",
  "r9",
  "r10",
  "r11",
  "cp",
  "dp",
  "sp",
  "lr",
  "et",
  "ed",
  "kep",
  "ksp",
  "spc",
  "sed",
  "ssr"
};

void Thread::finalize()
{
  scheduler = &getParent().getParent()->getParent()->getScheduler();
}

void Thread::dump() const
{
  std::cout << std::hex;
  for (unsigned i = 0; i < NUM_REGISTERS; i++) {
    std::cout << getRegisterName(i) << ": 0x" << regs[i] << "\n";
  }
  std::cout << "pc: 0x" << getParent().targetPc(pc) << "\n";
  std::cout << std::dec;
}

void Thread::schedule()
{
  getParent().getParent()->getParent()->schedule(*this);
}

void Thread::takeEvent()
{
  getParent().getParent()->getParent()->takeEvent(*this);
}

bool Thread::hasPendingEvent() const
{
  return getParent().getParent()->getParent()->hasPendingEvent();
}

bool Thread::setSRSlowPath(sr_t enabled)
{
  if (enabled[EEBLE]) {
    EventableResourceList &EER = eventEnabledResources;
    for (EventableResourceList::iterator it = EER.begin(),
         end = EER.end(); it != end; ++it) {
      if ((*it)->seeOwnerEventEnable(time)) {
        return true;
      }
    }
  }
  if (enabled[IEBLE]) {
    EventableResourceList &IER = interruptEnabledResources;
    for (EventableResourceList::iterator it = IER.begin(),
         end = IER.end(); it != end; ++it) {
      if ((*it)->seeOwnerEventEnable(time)) {
        return true;
      }
    }
  }
  return false;
}

bool Thread::isExecuting() const
{
  return this == parent->getParent()->getParent()->getExecutingRunnable();
}

enum {
  SETC_INUSE_OFF = 0x0,
  SETC_INUSE_ON = 0x8,
  SETC_COND_FULL = 0x1,
  SETC_COND_AFTER = 0x9,
  SETC_COND_EQ = 0x11,
  SETC_COND_NEQ = 0x19,
  SETC_IE_MODE_EVENT = 0x2,
  SETC_IE_MODE_INTERRUPT = 0xa,
  SETC_RUN_STOPR = 0x7,
  SETC_RUN_STARTR = 0xf,
  SETC_RUN_CLRBUF = 0x17,
  SETC_MS_MASTER = 0x1007,
  SETC_MS_SLAVE = 0x100f,
  SETC_BUF_NOBUFFERS = 0x2007,
  SETC_BUF_BUFFERS = 0x200f,
  SETC_RDY_NOREADY = 0x3007,
  SETC_RDY_STROBED = 0x300f,
  SETC_RDY_HANDSHAKE = 0x3017,
  SETC_PORT_DATAPORT = 0x5007,
  SETC_PORT_CLOCKPORT = 0x500f,
  SETC_PORT_READYPORT = 0x5017
};

static void internalError(const Thread &thread, const char *file, int line) {
  std::cout << "Internal error in " << file << ":" << line << "\n"
  << "Register state:\n";
  thread.dump();
  std::abort();
}

static inline uint32_t maskSize(uint32_t x)
{
  assert((x & (x + 1)) == 0 && "Not a valid mkmsk mask");
  return 32 - countLeadingZeros(x);
}

static Resource::Condition setCCondition(uint32_t Val)
{
  switch (Val) {
  default: assert(0 && "Unexpected setc value");
  case SETC_COND_FULL:
    return Resource::COND_FULL;
  case SETC_COND_AFTER:
    return Resource::COND_AFTER;
  case SETC_COND_EQ:
    return Resource::COND_EQ;
  case SETC_COND_NEQ:
    return Resource::COND_NEQ;
  }
}

static Port::ReadyMode
getPortReadyMode(uint32_t val)
{
  switch (val) {
  default: assert(0 && "Unexpected ready mode");
  case SETC_RDY_NOREADY: return Port::NOREADY;
  case SETC_RDY_STROBED: return Port::STROBED;
  case SETC_RDY_HANDSHAKE: return Port::HANDSHAKE;
  }
}

static Port::MasterSlave
getMasterSlave(uint32_t val)
{
  switch (val) {
  default: assert(0 && "Unexpected master slave value");
  case SETC_MS_MASTER: return Port::MASTER;
  case SETC_MS_SLAVE: return Port::SLAVE;
  }
}

static Port::PortType
getPortType(uint32_t val)
{
  switch (val) {
  default: assert(0 && "Unexpected port type");
  case SETC_PORT_DATAPORT: return Port::DATAPORT;
  case SETC_PORT_CLOCKPORT: return Port::CLOCKPORT;
  case SETC_PORT_READYPORT: return Port::READYPORT;
  }
}

bool Thread::
setC(ticks_t time, ResourceID resID, uint32_t val)
{
  Resource *res = getParent().getResourceByID(resID);
  if (!res) {
    return false;
  }
  if (val == SETC_INUSE_OFF || val == SETC_INUSE_ON)
    return res->setCInUse(*this, val == SETC_INUSE_ON, time);
  if (!res->isInUse())
    return false;
  switch (val) {
  default:
    internalError(*this, __FILE__, __LINE__); // TODO
  case SETC_IE_MODE_EVENT:
  case SETC_IE_MODE_INTERRUPT:
    {
      if (!res->isEventable())
        return false;
      EventableResource *ER = static_cast<EventableResource *>(res);
      ER->setInterruptMode(*this, val == SETC_IE_MODE_INTERRUPT);
      break;
    }
  case SETC_COND_FULL:
  case SETC_COND_AFTER:
  case SETC_COND_EQ:
  case SETC_COND_NEQ:
    {
      if (!res->setCondition(*this, setCCondition(val), time))
        return false;
      break;
    }
  case SETC_RUN_STARTR:
  case SETC_RUN_STOPR:
    {
      if (res->getType() != RES_TYPE_CLKBLK)
        return false;
      ClockBlock *clock = static_cast<ClockBlock*>(res);
      if (val == SETC_RUN_STARTR)
        clock->start(*this, time);
      else
        clock->stop(*this, time);
      break;
    }
  case SETC_MS_MASTER:
  case SETC_MS_SLAVE:
    if (res->getType() != RES_TYPE_PORT)
      return false;
    return static_cast<Port*>(res)->setMasterSlave(*this, getMasterSlave(val),
                                                   time);
  case SETC_BUF_BUFFERS:
  case SETC_BUF_NOBUFFERS:
    if (res->getType() != RES_TYPE_PORT)
      return false;
    return static_cast<Port*>(res)->setBuffered(*this, val == SETC_BUF_BUFFERS,
                                                time);
  case SETC_RDY_NOREADY:
  case SETC_RDY_STROBED:
  case SETC_RDY_HANDSHAKE:
    if (res->getType() != RES_TYPE_PORT)
      return false;
    return static_cast<Port*>(res)->setReadyMode(*this, getPortReadyMode(val),
                                                 time);
  case SETC_PORT_DATAPORT:
  case SETC_PORT_CLOCKPORT:
  case SETC_PORT_READYPORT:
    if (res->getType() != RES_TYPE_PORT)
      return false;
    return static_cast<Port*>(res)->setPortType(*this, getPortType(val), time);
  case SETC_RUN_CLRBUF:
    {
      if (res->getType() != RES_TYPE_PORT)
        return false;
      static_cast<Port*>(res)->clearBuf(*this, time);
      break;
    }
  }
  return true;
}

#include "InstructionMacrosCommon.h"

#define THREAD thread
#define CORE THREAD.getParent()
//#define ERROR() internalError(THREAD, __FILE__, __LINE__);
#define ERROR() std::abort();
#define OP(n) (CORE.operands[THREAD.pc].ops[(n)])
#define LOP(n) (CORE.operands[THREAD.pc].lops[(n)])
#define TRACE(...) \
do { \
if (tracing) { \
Tracer::get().trace(THREAD, __VA_ARGS__); \
} \
} while(0)
#define TRACE_REG_WRITE(register, value) \
do { \
if (tracing) { \
Tracer::get().regWrite(register, value); \
} \
} while(0)
#define TRACE_END() \
do { \
if (tracing) { \
Tracer::get().traceEnd(); \
} \
} while(0)
#define EMIT_INSTRUCTION_FUNCTIONS
#include "InstructionGenOutput.inc"
#undef EMIT_INSTRUCTION_FUNCTIONS


#define INSTRUCTION_CYCLES 4

template<bool tracing>
JITReturn Instruction_TSETMR_2r(Thread &thread) {
  TRACE("tsetmr ", Register(OP(0)), ", ", SrcRegister(OP(1)));
  THREAD.time += INSTRUCTION_CYCLES;
  Synchroniser *sync = THREAD.getSync();
  if (sync) {
    sync->master().reg((OP(0))) = THREAD.reg(OP(1));
  } else {
    // Undocumented behaviour, possibly incorrect. However this seems to
    // match the simulator.
    THREAD.reg(OP(0)) = THREAD.reg(OP(1));
  }
  THREAD.pc++;
  TRACE_END();
  return JIT_RETURN_CONTINUE;
}

template<bool tracing>
JITReturn Instruction_RUN_JIT(Thread &thread) {
  CORE.runJIT(THREAD.pendingPc);
  THREAD.pc = THREAD.pendingPc;
  return JIT_RETURN_END_TRACE;
}

template<bool tracing>
JITReturn Instruction_DECODE(Thread &thread) {
  InstructionOpcode opc;
  instructionDecode(CORE, THREAD.pc, opc, CORE.operands[THREAD.pc]);
  instructionTransform(opc, CORE.operands[THREAD.pc], CORE, THREAD.pc);
  if (CORE.invalidationInfo[THREAD.pc] == Core::INVALIDATE_NONE) {
    CORE.invalidationInfo[THREAD.pc] = Core::INVALIDATE_CURRENT;
  }
  if (instructionProperties[opc].size == 4) {
    CORE.invalidationInfo[THREAD.pc + 1] = Core::INVALIDATE_CURRENT_AND_PREVIOUS;
  } else {
    assert(instructionProperties[opc].size == 2);
  }
  static OPCODE_TYPE opcodeMap[] = {
#define EMIT_INSTRUCTION_LIST
#define DO_INSTRUCTION(inst) & Instruction_ ## inst <tracing>,
#include "InstructionGenOutput.inc"
#undef EMIT_INSTRUCTION_LIST
  };
  CORE.opcode[THREAD.pc] = opcodeMap[opc];
  return JIT_RETURN_END_TRACE;
}

#undef THREAD
#undef CORE
#undef ERROR
#undef OP
#undef LOP
#undef TRACE
#undef TRACE_REG_WRITE
#undef TRACE_END

void Thread::run(ticks_t time)
{
  OPCODE_TYPE *opcode = getParent().opcode;
  
  while (1) {
    if ((*opcode[pc])(*this) == JIT_RETURN_END_THREAD_EXECUTION)
      return;
  }
}

template<bool tracing>
void initInstructionCacheAux(Core &c)
{
  c.initCache(&Instruction_DECODE<tracing>,
              &Instruction_ILLEGAL_PC<tracing>,
              &Instruction_ILLEGAL_PC_THREAD<tracing>,
              &Instruction_SYSCALL<tracing>,
              &Instruction_EXCEPTION<tracing>,
              &Instruction_RUN_JIT<tracing>);
}

void initInstructionCache(Core &c)
{
  if (Tracer::get().getTracingEnabled())
    initInstructionCacheAux<true>(c);
  else
    initInstructionCacheAux<false>(c);
}
