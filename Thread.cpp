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

static Resource *
checkResource(Core &state, ResourceID id)
{
  Resource *res = state.getResourceByID(id);
  if (!res || !res->isInUse())
    return 0;
  return res;
}

static inline Resource *
checkResource(Thread &thread, ResourceID id)
{
  return checkResource(thread.getParent(), id);
}

static Synchroniser *
checkSync(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (res->getType() != RES_TYPE_SYNC)
    return 0;
  return static_cast<Synchroniser *>(res);
}

static inline Synchroniser *
checkSync(Thread &thread, ResourceID id)
{
  return checkSync(thread.getParent(), id);
}

static Thread *
checkThread(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (res->getType() != RES_TYPE_THREAD)
    return 0;
  return static_cast<Thread *>(res);
}

static inline Thread *
checkThread(Thread &thread, ResourceID id)
{
  return checkThread(thread.getParent(), id);
}

static Lock *
checkLock(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (res->getType() != RES_TYPE_LOCK)
    return 0;
  return static_cast<Lock *>(res);
}

static inline Lock *
checkLock(Thread &thread, ResourceID id)
{
  return checkLock(thread.getParent(), id);
}

static Chanend *
checkChanend(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (res->getType() != RES_TYPE_CHANEND)
    return 0;
  return static_cast<Chanend *>(res);
}

static inline Chanend *
checkChanend(Thread &thread, ResourceID id)
{
  return checkChanend(thread.getParent(), id);
}

static Timer *
checkTimer(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (res->getType() != RES_TYPE_TIMER)
    return 0;
  return static_cast<Timer *>(res);
}

static inline Timer *
checkTimer(Thread &thread, ResourceID id)
{
  return checkTimer(thread.getParent(), id);
}

static Port *
checkPort(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (res->getType() != RES_TYPE_PORT)
    return 0;
  return static_cast<Port *>(res);
}

static inline Port *
checkPort(Thread &thread, ResourceID id)
{
  return checkPort(thread.getParent(), id);
}

static EventableResource *
checkEventableResource(Core &state, ResourceID id)
{
  Resource *res = checkResource(state, id);
  if (!res)
    return 0;
  if (!res->isEventable())
    return 0;
  return static_cast<EventableResource *>(res);
}

static inline EventableResource *
checkEventableResource(Thread &thread, ResourceID id)
{
  return checkEventableResource(thread.getParent(), id);
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

const uint32_t CLK_REF = 0x1;

bool Thread::
setClock(ResourceID resID, uint32_t val, ticks_t time)
{
  Core &state = getParent();
  Resource *res = checkResource(state, resID);
  if (!res) {
    return false;
  }
  switch (res->getType()) {
  default: return false;
  case RES_TYPE_CLKBLK:
    {
      ClockBlock *c = static_cast<ClockBlock*>(res);
      if (val == CLK_REF) {
        c->setSourceRefClock(*this, time);
        return true;
      }
      Port *p = checkPort(state, val);
      if (!p || p->getPortWidth() != 1)
        return false;
      return c->setSource(*this, p, time);
    }
  case RES_TYPE_PORT:
    {
      Resource *source = state.getResourceByID(val);
      if (!source)
        return false;
      if (source->getType() != RES_TYPE_CLKBLK)
        return false;
      static_cast<Port*>(res)->setClk(*this, static_cast<ClockBlock*>(source),
                                      time);
      return true;
    }
    break;
  }
  return true;
}

bool Thread::
threadSetReady(ResourceID resID, uint32_t val, ticks_t time)
{
  Core &state = getParent();
  Resource *res = checkResource(state, resID);
  Port *ready = checkPort(state, val);
  if (!res || !ready) {
    return false;
  }
  return res->setReady(*this, ready, time);
}

#include "InstructionMacrosCommon.h"

#ifdef DIRECT_THREADED
#define INST(s) s ## _label
#define ENDINST goto *(opcode[PC] + (char*)&&INST(INITIALIZE))
#define OPCODE(s) ((char*)&&INST(s) - (char*)&&INST(INITIALIZE))
#define START_DISPATCH_LOOP ENDINST;
#define END_DISPATCH_LOOP
#else
#define INST(inst) case (inst)
#define ENDINST break
#define OPCODE(s) s
#define START_DISPATCH_LOOP while(1) { switch (opcode[PC]) {
#define END_DISPATCH_LOOP } }
#endif

#define STORE_WORD(value, addr) \
do { \
  INVALIDATE_WORD(addr); \
  core->storeWord(value, addr); \
} while(0)
#define STORE_SHORT(value, addr) \
do { \
  INVALIDATE_SHORT(addr); \
  core->storeShort(value, addr); \
} while(0)
#define STORE_BYTE(value, addr) \
do { \
  INVALIDATE_BYTE(addr); \
  core->storeByte(value, addr); \
} while(0)

#define SAVE_CACHED() \
do { \
  this->pc = PC;\
} while(0)
#define REG(Num) this->regs[Num]
#define IMM(Num) (Num)
#define THREAD (*this)
#define CORE (*core)
#define PC this->pc
#define TIME this->time
#define OP(n) (operands[PC].ops[(n)])
#define LOP(n) (operands[PC].lops[(n)])
#define EXCEPTION(et, ed) \
do { \
  PC = exception(THREAD, PC, et, ed); \
  NEXT_THREAD(PC); \
} while(0);
#define ERROR() \
do { \
  SAVE_CACHED(); \
  internalError(*this, __FILE__, __LINE__); \
} while(0)
#define TRACE(...) \
do { \
  if (tracing) { \
    SAVE_CACHED(); \
    Tracer::get().trace(*this, __VA_ARGS__); \
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
#define DESCHEDULE(pc) \
do { \
  SAVE_CACHED(); \
  this->waiting() = true; \
  return; \
} while(0)
#define PAUSE_ON(pc, resource) \
do { \
  SAVE_CACHED(); \
  this->waiting() = true; \
  this->pausedOn = resource; \
  return; \
} while(0)
#define NEXT_THREAD(pc) \
do { \
  if (sys.hasTimeSliceExpired(TIME)) { \
    SAVE_CACHED(); \
    sys.schedule(*this); \
    return; \
  } \
} while(0)
#define TAKE_EVENT(pc) \
do { \
  SAVE_CACHED(); \
  sys.takeEvent(*this); \
  sys.schedule(*this); \
  return; \
} while(0)
#define SETSR(bits, pc) \
do { \
  SAVE_CACHED(); \
  if (this->setSR(bits)) { \
    sys.takeEvent(*this); \
  } \
  sys.schedule(*this); \
  return; \
} while(0)

#define INSTRUCTION_CYCLES 4

void Thread::run(ticks_t time)
{
  if (Tracer::get().getTracingEnabled())
    runAux<true>(time);
  else
    runAux<false>(time);
}

template <bool tracing>
void Thread::runAux(ticks_t time) {
  SystemState &sys = *getParent().getParent()->getParent();
  Core *core = &this->getParent();
  OPCODE_TYPE *opcode = core->opcode;
  Operands *operands = core->operands;

    // The main dispatch loop. On backward branches and indirect jumps we call
  // NEXT_THREAD() to ensure one thread which never pauses cannot starve the
  // other threads.
  START_DISPATCH_LOOP
  INST(INITIALIZE):
    getParent().initCache(OPCODE(DECODE), OPCODE(ILLEGAL_PC),
                          OPCODE(ILLEGAL_PC_THREAD), OPCODE(SYSCALL),
                          OPCODE(EXCEPTION));
    ENDINST;
#define EMIT_INSTRUCTION_DISPATCH
#include "InstructionGenOutput.inc"
#undef EMIT_INSTRUCTION_DISPATCH
  // Two operand short.
  INST(TSETMR_2r):
    {
      TRACE("tsetmr ", Register(OP(0)), ", ", SrcRegister(OP(1)));
      TIME += INSTRUCTION_CYCLES;
      Synchroniser *sync = this->getSync();
      if (sync) {
        sync->master().reg(IMM(OP(0))) = REG(OP(1));
      } else {
        // Undocumented behaviour, possibly incorrect. However this seems to
        // match the simulator.
        REG(OP(0)) = REG(OP(1));
      }
      PC++;
      TRACE_END();
    }
    ENDINST;
      
  // Zero operand short
  INST(SSYNC_0r):
    {
      TRACE("ssync");
      TIME += INSTRUCTION_CYCLES;
      Synchroniser *sync = this->getSync();
      // May schedule / deschedule threads
      if (!sync) {
        // TODO is this right?
        DESCHEDULE(PC);
      } else {
        switch (sync->ssync(*this)) {
        case Synchroniser::SYNC_CONTINUE:
          PC++;
          break;
        case Synchroniser::SYNC_DESCHEDULE:
          TRACE_END();
          PAUSE_ON(PC, sync);
          break;
        case Synchroniser::SYNC_KILL:
          {
            free();
            TRACE_END();
            DESCHEDULE(PC);
          }
        }
      }
      TRACE_END();
    }
    ENDINST;
  INST(FREET_0r):
    {
      TRACE("freet");
      if (this->getSync()) {
        // TODO check expected behaviour
        ERROR();
      }
      free();
      TRACE_END();
      DESCHEDULE(PC);
    }
    ENDINST;
  
  // Pseudo instructions.
  INST(SYSCALL):
    int retval;
    this->pc = PC;
    switch (SyscallHandler::doSyscall(*this, retval)) {
    case SyscallHandler::EXIT:
      throw (ExitException(retval));
    case SyscallHandler::DESCHEDULE:
      DESCHEDULE(PC);
      break;
    case SyscallHandler::CONTINUE:
      uint32_t target = TO_PC(REG(LR));
      if (CHECK_PC(target)) {
        PC = target;
        NEXT_THREAD(PC);
      } else {
        EXCEPTION(ET_ILLEGAL_PC, REG(LR));
      }
      break;
    }
    ENDINST;
  INST(EXCEPTION):
    this->pc = PC;
    SyscallHandler::doException(*this);
    throw (ExitException(1));
    ENDINST;
  INST(ILLEGAL_PC):
    EXCEPTION(ET_ILLEGAL_PC, FROM_PC(PC));
    ENDINST;
  INST(ILLEGAL_PC_THREAD):
    EXCEPTION(ET_ILLEGAL_PC, this->illegal_pc);
    ENDINST;
  INST(ILLEGAL_INSTRUCTION):
    EXCEPTION(ET_ILLEGAL_INSTRUCTION, 0);
    ENDINST;
  INST(DECODE):
    {
      InstructionOpcode opc;
      // Try and JIT the instruction stream.
      if (!tracing && JIT::compile(CORE, PC << 1, operands[PC].func)) {
        opc = JIT_INSTRUCTION;
      } else {
        // Otherwise fall back to the interpreter.
        // TODO avoid decoding the instruction twice if JIT fails.
        uint16_t low = core->loadShort(PC << 1);
        uint16_t high = 0;
        bool highValid;
        if (CHECK_ADDR((PC + 1) << 1)) {
          high = core->loadShort((PC + 1) << 1);
          highValid = true;
        } else {
          highValid = false;
        }
        instructionDecode(low, high, highValid, opc, operands[PC]);
        instructionTransform(opc, operands[PC], CORE, PC);
      }
      // TODO handle word size instructions properly.
      if (CORE.invalidationInfo[PC] == Core::INVALIDATE_NONE) {
        CORE.invalidationInfo[PC] = Core::INVALIDATE_CURRENT;
      }
      // TODO check if the instruction was word size before this.
      if (CHECK_ADDR((PC + 1) << 1)) {
        CORE.invalidationInfo[PC + 1] = Core::INVALIDATE_CURRENT_AND_PREVIOUS;
      }
#ifdef DIRECT_THREADED
      static OPCODE_TYPE opcodeMap[] = {
#define EMIT_INSTRUCTION_LIST
#define DO_INSTRUCTION(inst) OPCODE(inst),
#include "InstructionGenOutput.inc"
#undef EMIT_INSTRUCTION_LIST
#undef DO_INSTRUCTION
      };
      opcode[PC] = opcodeMap[opc];
#else
      opcode[PC] = opc;
#endif
      // Reexecute current instruction.
      ENDINST;
    }
  INST(JIT_INSTRUCTION):
    if (operands[PC].func(THREAD))
      NEXT_THREAD(PC);
    ENDINST;
  END_DISPATCH_LOOP
}
