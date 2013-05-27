// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
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
#include "JIT.h"
#include "CRC.h"
#include "InstructionHelpers.h"
#include "Synchroniser.h"
#include "Chanend.h"
#include "ClockBlock.h"
#include "InstructionOpcode.h"
#include "InstructionProperties.h"
#include <iostream>
#include <cstdlib>

using namespace axe;
using namespace Register;

Thread::Thread() :
  Resource(RES_TYPE_THREAD),
  parent(0),
  scheduler(0)
{
  time = 0;
  pc = 0;
  regs[KEP] = 0;
  regs[KSP] = 0;
  regs[SPC] = 0;
  regs[SED] = 0;
  regs[ET] = 0;
  regs[ED] = 0;
  eeble() = false;
  ieble() = false;
  setInUse(false);
}

void Thread::setParent(Core &p)
{
  parent = &p;
  decodeCache = parent->getRamDecodeCache();
}

void Thread::finalize()
{
  scheduler = &getParent().getParent()->getParent()->getScheduler();
}

void Thread::seeRamDecodeCacheChange()
{
  if (decodeCache.base ==
      getParent().getParent()->getParent()->getRomDecodeCache().base)
    return;
  decodeCache = getParent().getRamDecodeCache();
}

void Thread::runJIT(uint32_t pc)
{
  if (isInRam())
    getParent().runJIT(pc);
}

void Thread::dump() const
{
  std::cout << std::hex;
  for (unsigned i = 0; i < NUM_REGISTERS; i++) {
    std::cout << getRegisterName(i) << ": 0x" << regs[i] << "\n";
  }
  std::cout << "pc: 0x" << getRealPc() << "\n";
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
    for (EventableResource *res : EER) {
      if (res->seeOwnerEventEnable(time)) {
        return true;
      }
    }
  }
  if (enabled[IEBLE]) {
    EventableResourceList &IER = interruptEnabledResources;
    for (EventableResource *res : IER) {
      if (res->seeOwnerEventEnable(time)) {
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

bool Thread::isInRam() const
{
  return decodeCache.base == parent->getRamDecodeCache().base;
}

bool Thread::tryUpdateDecodeCache(uint32_t address)
{
  assert((address & 1) == 0);
  if (!decodeCache.contains(address)) {
    const DecodeCache::State *newDecodeCache =
      getParent().getDecodeCacheContaining(address);
    if (!newDecodeCache)
      return false;
    decodeCache = *newDecodeCache;
  }
  return true;
}

void Thread::setPcFromAddress(uint32_t address)
{
  bool updateOk = tryUpdateDecodeCache(address);
  assert(updateOk);
  (void)updateOk;
  pc = decodeCache.toPc(address);
}

uint32_t Thread::getRealPc() const
{
  if (pc == parent->getInterpretOneAddr() ||
      pc == parent->getRunJitAddr())
    return fromPc(pendingPc);
  return fromPc(pc);
}

enum {
  SETC_MODE_INUSE = 0x0,
  SETC_MODE_COND = 0x1,
  SETC_MODE_IE_MODE = 0x2,
  SETC_MODE_DRIVE = 0x3,
  SETC_MODE_LONG = 0x7
};

const uint32_t SETC_MODE_SHIFT = 0x0;
const uint32_t SETC_MODE_SIZE = 0x3;

const uint32_t SETC_VALUE_SHIFT = 0x3;
const uint32_t SETC_VALUE_SIZE = 0x9;

enum {
  SETC_LMODE_RUN = 0x0,
  SETC_LMODE_MS = 0x1,
  SETC_LMODE_BUF = 0x2,
  SETC_LMODE_RDY = 0x3,
  SETC_LMODE_SDELAY = 0x4,
  SETC_LMODE_PORT = 0x5,
  SETC_LMODE_INV = 0x6,
  SETC_LMODE_PIN_DELAY = 0x7,
  SETC_LMODE_FALL_DELAY = 0x8,
  SETC_LMODE_RISE_DELAY = 0x9
};

const uint32_t SETC_LMODE_SHIFT = 0xc;
const uint32_t SETC_LMODE_SIZE = 0x4;

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
  SETC_SDELAY_NOSDELAY = 0x4007,
  SETC_SDELAY_SDELAY = 0x400f,
  SETC_PORT_DATAPORT = 0x5007,
  SETC_PORT_CLOCKPORT = 0x500f,
  SETC_PORT_READYPORT = 0x5017,
  SETC_INV_NOINVERT = 0x6007,
  SETC_INV_INVERT = 0x600f
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

static Edge::Type getEdgeTypeFromLMode(uint32_t val)
{
  switch (val) {
  default: assert(0 && "Unexpected LMode");
  case SETC_LMODE_RISE_DELAY:
    return Edge::RISING;
  case SETC_LMODE_FALL_DELAY:
    return Edge::FALLING;
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
  if (extractBits(val, SETC_MODE_SHIFT, SETC_MODE_SIZE) == SETC_MODE_LONG) {
    uint32_t lmode = extractBits(val, SETC_LMODE_SHIFT, SETC_LMODE_SIZE);
    uint32_t valField = extractBits(val, SETC_VALUE_SHIFT, SETC_VALUE_SIZE);
    switch (lmode) {
    default: break;
    case SETC_LMODE_PIN_DELAY:
      if (res->getType() == RES_TYPE_PORT) {
        Port *port = static_cast<Port*>(res);
        return port->setPinDelay(*this, valField, time);
      }
      return false;
    case SETC_LMODE_FALL_DELAY:
    case SETC_LMODE_RISE_DELAY:
      if (res->getType() == RES_TYPE_CLKBLK) {
        ClockBlock *clock = static_cast<ClockBlock*>(res);
        return clock->setEdgeDelay(*this, getEdgeTypeFromLMode(lmode), valField,
                                   time);
      }
      return false;
    }
  }
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
  case SETC_INV_INVERT:
  case SETC_INV_NOINVERT:
    if (res->getType() != RES_TYPE_PORT)
      return false;
    return static_cast<Port*>(res)->setPortInv(*this, val == SETC_INV_INVERT,
                                               time);
  case SETC_SDELAY_NOSDELAY:
  case SETC_SDELAY_SDELAY:
    {
      if (res->getType() != RES_TYPE_PORT)
        return false;
      Edge::Type samplingEdge = (val == SETC_SDELAY_SDELAY) ? Edge::FALLING :
                                                              Edge::RISING;
      static_cast<Port*>(res)->setSamplingEdge(*this, samplingEdge, time);
      break;
    }
  }
  return true;
}

#include "InstructionMacrosCommon.h"

#define THREAD thread
#define CORE THREAD.getParent()
#define CHECK_ADDR_RAM(addr) CORE.isValidRamAddress(addr)
#define CHECK_PC(addr) (((addr) >> (CORE.getRamSizeLog2() - 1)) == 0)
//#define ERROR() internalError(THREAD, __FILE__, __LINE__);
#define ERROR() std::abort();
#define OP(n) (THREAD.getOperands(THREAD.pc).ops[(n)])
#define LOP(n) (THREAD.getOperands(THREAD.pc).lops[(n)])
#define TRACE() \
do { \
if (tracing) { \
CORE.getTracer().trace(THREAD); \
} \
} while(0)
#define TRACE_REG_WRITE(register, value) \
do { \
if (tracing) { \
CORE.getTracer().regWrite(register, value); \
} \
} while(0)
#define TRACE_END() \
do { \
if (tracing) { \
CORE.getTracer().traceEnd(); \
} \
} while(0)
#define EMIT_INSTRUCTION_FUNCTIONS
#include "InstructionGenOutput.inc"
#undef EMIT_INSTRUCTION_FUNCTIONS


#define INSTRUCTION_CYCLES 4

template<bool tracing>
InstReturn Instruction_TSETMR_2r(Thread &thread) {
  TRACE();
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
  return InstReturn::CONTINUE;
}

template<bool tracing> InstReturn Instruction_RUN_JIT(Thread &thread) {
  CORE.runJIT(THREAD.pendingPc);
  THREAD.pc = THREAD.pendingPc;
  return InstReturn::END_TRACE;
}

template<bool tracing> InstReturn Instruction_BREAKPOINT(Thread &thread) {
  // Arrange for the current instruction to be interpreted so we don't hit the
  // breakpoint again when continuing.
  THREAD.pendingPc = THREAD.pc;
  THREAD.pc = CORE.getInterpretOneAddr();
  THREAD.waiting() = true;
  THREAD.schedule();
  throw (BreakpointException(TIME, THREAD));
}

template<bool tracing> InstReturn Instruction_INTERPRET_ONE(Thread &thread) {
  THREAD.pc = THREAD.pendingPc;
  return THREAD.interpretOne();
}

template<bool tracing> InstReturn Instruction_DECODE(Thread &thread);

static OPCODE_TYPE opcodeMap[] = {
#define EMIT_INSTRUCTION_LIST
#define DO_INSTRUCTION(inst) & Instruction_ ## inst <false>,
#include "InstructionGenOutput.inc"
#undef DO_INSTRUCTION
#undef EMIT_INSTRUCTION_LIST
};

static OPCODE_TYPE opcodeMapTracing[] = {
#define EMIT_INSTRUCTION_LIST
#define DO_INSTRUCTION(inst) & Instruction_ ## inst <true>,
#include "InstructionGenOutput.inc"
#undef DO_INSTRUCTION
#undef EMIT_INSTRUCTION_LIST
};

template<bool tracing> InstReturn Instruction_DECODE(Thread &thread) {
  InstructionOpcode opc;
  Operands ops;
  uint32_t address = THREAD.fromPc(THREAD.pc);
  instructionDecode(CORE, address, opc, ops);
  instructionTransform(opc, ops, CORE, address);
  THREAD.setOpcode(THREAD.pc, (tracing ? opcodeMapTracing : opcodeMap)[opc], ops,
                   instructionProperties[opc].size);
  return InstReturn::END_TRACE;
}

#undef THREAD
#undef CORE
#undef ERROR
#undef OP
#undef LOP
#undef TRACE
#undef TRACE_REG_WRITE
#undef TRACE_END

InstReturn Thread::interpretOne()
{
  Operands &ops = decodeCache.operands[pc];
  Operands oldOps = ops;
  InstructionOpcode opc;
  uint32_t address = fromPc(pc);
  instructionDecode(*parent, address, opc, ops, true /*ignoreBreakpoints*/);
  instructionTransform(opc, ops, *parent, address);
  bool tracing = decodeCache.tracingEnabled;
  InstReturn retval = (*(tracing ? opcodeMapTracing : opcodeMap)[opc])(*this);
  ops = oldOps;
  return retval;
}

void Thread::run(ticks_t time)
{
  while (1) {
    if ((*decodeCache.opcode[pc])(*this) == InstReturn::END_THREAD_EXECUTION)
      return;
  }
}

OPCODE_TYPE axe::getInstruction_DECODE(bool tracing) {
  if (tracing)
    return &Instruction_DECODE<true>;
  else
    return &Instruction_DECODE<false>;
}

OPCODE_TYPE axe::getInstruction_ILLEGAL_PC(bool tracing) {
  if (tracing)
    return &Instruction_ILLEGAL_PC<true>;
  else
    return &Instruction_ILLEGAL_PC<false>;
}

OPCODE_TYPE axe::getInstruction_ILLEGAL_PC_THREAD(bool tracing) {
  if (tracing)
    return &Instruction_ILLEGAL_PC_THREAD<true>;
  else
    return &Instruction_ILLEGAL_PC_THREAD<false>;
}

OPCODE_TYPE axe::getInstruction_RUN_JIT(bool tracing) {
  if (tracing)
    return &Instruction_RUN_JIT<true>;
  else
    return &Instruction_RUN_JIT<false>;
}

OPCODE_TYPE axe::getInstruction_INTERPRET_ONE(bool tracing) {
  if (tracing)
    return &Instruction_INTERPRET_ONE<true>;
  else
    return &Instruction_INTERPRET_ONE<false>;
}
