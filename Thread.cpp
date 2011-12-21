// Copyright (c) 2011, Richard Osborne, All rights reserved
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
#include <iostream>
#include <climits>

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

enum ProcessorState {
  PS_RAM_BASE = 0x00b,
  PS_VECTOR_BASE = 0x10b
};

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

uint32_t Thread::
exception(Core &state, uint32_t pc, int et, uint32_t ed)
{
  uint32_t sed = regs[ED];
  uint32_t spc = state.targetPc(pc);
  uint32_t ssr = sr.to_ulong();
  
  if (Tracer::get().getTracingEnabled()) {
    Tracer::get().exception(*this, et, ed, sed, ssr, spc);
  }
  
  regs[SSR] = sed;
  regs[SPC] = spc;
  regs[SED] = sed;
  ink() = true;
  eeble() = false;
  ieble() = false;
  regs[ET] = et;
  regs[ED] = ed;
  
  uint32_t newPc = state.physicalAddress(regs[KEP]);
  if (et == ET_KCALL)
    newPc += 64;
  if ((newPc & 1) || !state.isValidAddress(newPc)) {
    std::cout << "Error: unable to handle exception (invalid kep)\n";
    std::abort();
  }
  return newPc >> 1;
}

static void internalError(const Thread &thread, const char *file, int line) {
  std::cout << "Internal error in " << file << ":" << line << "\n"
  << "Register state:\n";
  thread.dump();
  std::abort();
}

static inline uint32_t signExtend(uint32_t value, uint32_t amount)
{
  if (amount == 0 || amount > 31) {
    return value;
  }
  return (uint32_t)(((int32_t)(value << (32 - amount))) >> (32 - amount));
}

static inline uint32_t zeroExtend(uint32_t value, uint32_t amount)
{
  if (amount == 0 || amount > 31) {
    return value;
  }
  return (value << (32 - amount)) >> (32 - amount);
}

template <typename T> uint32_t crc(uint32_t checksum, T data, uint32_t poly)
{
  for (unsigned i = 0; i < sizeof(T) * CHAR_BIT; i++) {
    int xorBit = (checksum & 1);
    
    checksum  = (checksum >> 1) | ((data & 1) << 31);
    data = data >> 1;
    
    if (xorBit)
      checksum = checksum ^ poly;
  }
  return checksum;
}

static uint32_t crc32(uint32_t checksum, uint32_t data, uint32_t poly)
{
  return crc<uint32_t>(checksum, data, poly);
}

static uint32_t crc8(uint32_t checksum, uint8_t data, uint32_t poly)
{
  return crc<uint8_t>(checksum, data, poly);
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

#ifdef DIRECT_THREADED
#define START_DISPATCH_LOOP goto *opcode[PC];
#define END_DISPATCH_LOOP
#define INST(s) s ## _label
#define ENDINST goto *opcode[PC]
#define OPCODE(s) &&INST(s)
#else
#define START_DISPATCH_LOOP while(1) { switch (opcode[PC]) {
#define END_DISPATCH_LOOP } }
#define INST(inst) case (inst)
#define ENDINST break
#define OPCODE(s) s
#endif

#define LOAD_WORD(addr) core->loadWord(addr)
#define LOAD_SHORT(addr) core->loadShort(addr)
#define LOAD_BYTE(addr) core->loadByte(addr)
#define INVALIDATE_WORD(addr) \
do { \
  opcode[(addr) >> 1] = OPCODE(DECODE); \
  opcode[1 + ((addr) >> 1)] = OPCODE(DECODE); \
} while(0)
#define INVALIDATE_SHORT(addr) \
do { \
  opcode[(addr) >> 1] = OPCODE(DECODE); \
} while(0)
#define INVALIDATE_BYTE(addr) INVALIDATE_SHORT(addr)

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
#define PC pc
#define TIME this->time
#define TO_PC(addr) (core->physicalAddress(addr) >> 1)
#define FROM_PC(addr) core->virtualAddress((addr) << 1)
#define CHECK_PC(addr) ((addr) < (core->ram_size << 1))
#define OP(n) (operands[PC].ops[(n)])
#define LOP(n) (operands[PC].lops[(n)])
#define ADDR(addr) core->physicalAddress(addr)
#define PHYSICAL_ADDR(addr) core->physicalAddress(addr)
#define VIRTUAL_ADDR(addr) core->virtualAddress(addr)
#define CHECK_ADDR(addr) core->isValidAddress(addr)
#define CHECK_ADDR_SHORT(addr) (!((addr) & 1) && CHECK_ADDR(addr))
#define CHECK_ADDR_WORD(addr) (!((addr) & 3) && CHECK_ADDR(addr))
#define EXCEPTION(et, ed) \
do { \
  PC = exception(*core, PC, et, ed); \
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
  getParent().ensureCacheIsValid(OPCODE(DECODE), OPCODE(ILLEGAL_PC),
                                 OPCODE(ILLEGAL_PC_THREAD),
                                 OPCODE(SYSCALL), OPCODE(EXCEPTION));
  SystemState &sys = *getParent().getParent()->getParent();
  uint32_t pc = this->pc;
  Core *core = &this->getParent();
  OPCODE_TYPE *opcode = core->opcode;
  Operands *operands = core->operands;

    // The main dispatch loop. On backward branches and indirect jumps we call
  // NEXT_THREAD() to ensure one thread which never pauses cannot starve the
  // other threads.
  START_DISPATCH_LOOP
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
      uint16_t low = core->loadShort(PC << 1);
      uint16_t high = 0;
      bool highValid;
      if (CHECK_ADDR((PC + 1) << 1)) {
        high = core->loadShort((PC + 1) << 1);
        highValid = true;
      } else {
        highValid = false;
      }
      InstructionOpcode opc;
      instructionDecode(low, high, highValid, opc, operands[PC]);
      switch (opc) {
      default:
        break;
      case ADD_2rus:
        if (OP(2) == 0) {
          opc = ADD_mov_2rus;
        }
        break;
      case STW_2rus:
      case LDW_2rus:
      case LDAWF_l2rus:
      case LDAWB_l2rus:
        OP(2) = OP(2) << 2;
        break;
      case STWDP_ru6:
      case STWSP_ru6:
      case LDWDP_ru6:
      case LDWSP_ru6:
      case LDAWDP_ru6:
      case LDAWSP_ru6:
      case LDWCP_ru6:
      case STWDP_lru6:
      case STWSP_lru6:
      case LDWDP_lru6:
      case LDWSP_lru6:
      case LDAWDP_lru6:
      case LDAWSP_lru6:
      case LDWCP_lru6:
        OP(1) = OP(1) << 2;
        break;
      case EXTDP_u6:
      case ENTSP_u6:
      case EXTSP_u6:
      case RETSP_u6:
      case KENTSP_u6:
      case KRESTSP_u6:
      case LDAWCP_u6:
      case LDWCPL_u10:
      case EXTDP_lu6:
      case ENTSP_lu6:
      case EXTSP_lu6:
      case RETSP_lu6:
      case KENTSP_lu6:
      case KRESTSP_lu6:
      case LDAWCP_lu6:
      case LDWCPL_lu10:
        OP(0) = OP(0) << 2;
        break;
      case LDAPB_u10:
      case LDAPF_u10:
      case LDAPB_lu10:
      case LDAPF_lu10:
        OP(0) = OP(0) << 1;
        break;
      case SHL_2rus:
        if (OP(2) == 32) {
          opc = SHL_32_2rus;
        }
        break;
      case SHR_2rus:
        if (OP(2) == 32) {
          opc = SHR_32_2rus;
        }
        break;
      case ASHR_l2rus:
        if (OP(2) == 32) {
          opc = ASHR_32_l2rus;
        }
        break;
      case BRFT_ru6:
        OP(1) = PC + 1 + OP(1);
        if (!CHECK_PC(OP(1))) {
          opc = BRFT_illegal_ru6;
        }
        break;
      case BRBT_ru6:
        OP(1) = PC + 1 - OP(1);
        if (!CHECK_PC(OP(1))) {
          opc = BRBT_illegal_ru6;
        }
        break;
      case BRFU_u6:
        OP(0) = PC + 1 + OP(0);
        if (!CHECK_PC(OP(0))) {
          opc = BRFU_illegal_u6;
        }
        break;
      case BRBU_u6:
        OP(0) = PC + 1 - OP(0);
        if (!CHECK_PC(OP(0))) {
          opc = BRBU_illegal_u6;
        }
        break;
      case BRFF_ru6:
        OP(1) = PC + 1 + OP(1);
        if (!CHECK_PC(OP(1))) {
          opc = BRFF_illegal_ru6;
        }
        break;
      case BRBF_ru6:
        OP(1) = PC + 1 - OP(1);
        if (!CHECK_PC(OP(1))) {
          opc = BRBF_illegal_ru6;
        }
        break;
      case BLRB_u10:
        OP(0) = PC + 1 - OP(0);
        if (!CHECK_PC(OP(0))) {
          opc = BLRB_illegal_u10;
        }
        break;
      case BLRF_u10:
        OP(0) = PC + 1 + OP(0);
        if (!CHECK_PC(OP(0))) {
          opc = BLRF_illegal_u10;
        }
        break;
      case BRFT_lru6:
        OP(1) = PC + 2 + OP(1);
        if (!CHECK_PC(OP(1))) {
          opc = BRFT_illegal_lru6;
        }
        break;
      case BRBT_lru6:
        OP(1) = PC + 2 - OP(1);
        if (!CHECK_PC(OP(1))) {
          opc = BRBT_illegal_lru6;
        }
        break;
      case BRFU_lu6:
        OP(0) = PC + 2 + OP(0);
        if (!CHECK_PC(OP(0))) {
          opc = BRFU_illegal_lu6;
        }
        break;
      case BRBU_lu6:
        OP(0) = PC + 2 - OP(0);
        if (!CHECK_PC(OP(0))) {
          opc = BRBU_illegal_lu6;
        }
        break;
      case BRFF_lru6:
        OP(1) = PC + 2 + OP(1);
        if (!CHECK_PC(OP(1))) {
          opc = BRFF_illegal_lru6;
        }
        break;
      case BRBF_lru6:
        OP(1) = PC + 2 - OP(1);
        if (!CHECK_PC(OP(1))) {
          opc = BRBF_illegal_lru6;
        }
        break;
      case BLRB_lu10:
        OP(0) = PC + 2 - OP(0);
        if (!CHECK_PC(OP(0))) {
          opc = BLRB_illegal_lu10;
        }
        break;
      case BLRF_lu10:
        OP(0) = PC + 2 + OP(0);
        if (!CHECK_PC(OP(0))) {
          opc = BLRF_illegal_lu10;
        }
        break;
      case MKMSK_rus:
        OP(1) = makeMask(OP(1));
        if (!tracing) {
          opc = LDC_ru6;
        }
        break;
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
  END_DISPATCH_LOOP
}

