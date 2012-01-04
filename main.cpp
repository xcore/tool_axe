// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gelf.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/relaxng.h>
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <memory>
#include <climits>
#include <set>
#include <map>

#include "Trace.h"
#include "Resource.h"
#include "Core.h"
#include "SyscallHandler.h"
#include "SymbolInfo.h"
#include "Exceptions.h"
#include "BitManip.h"
#include "XE.h"
#include "ScopedArray.h"
#include "Config.h"
#include "Instruction.h"
#include "Node.h"
#include "SystemState.h"
#include "WaveformTracer.h"
#include "PeripheralRegistry.h"
#include "registerAllPeripherals.h"
#include "UartRx.h"
#include "Property.h"
#include "PortArg.h"

#define XCORE_ELF_MACHINE 0xB49E

const char configSchema[] =
#include "ConfigSchema.inc"
;

static void printUsage(const char *ProgName) {
  std::cout << "Usage: " << ProgName << " [options] filename\n";
  std::cout <<
"General Options:\n"
"  -help                       Display this information.\n"
"  --loopback PORT1 PORT2      Connect PORT1 to PORT2.\n"
"  --vcd FILE                  Write VCD trace to FILE.\n"
"  -t                          Enable instruction tracing.\n"
"\n"
"Peripherals:\n";
  for (PeripheralRegistry::iterator it = PeripheralRegistry::begin(),
       e = PeripheralRegistry::end(); it != e; ++it) {
    PeripheralDescriptor *periph = *it;
    std::cout << "  --" << periph->getName() << ' ';
    bool needComma = false;
    for (PeripheralDescriptor::iterator propIt = periph->properties_begin(),
         propE = periph->properties_end(); propIt != propE; ++propIt) {
      const PropertyDescriptor &prop = *propIt;
      if (needComma)
        std::cout << ',';
      std::cout << prop.getName() << '=';
      needComma = true;
    }
    std::cout << '\n';
  }
}

static void readSymbols(Elf *e, Elf_Scn *scn, const GElf_Shdr &shdr,
                        unsigned low, unsigned high,
                        std::auto_ptr<CoreSymbolInfo> &SI)
{
  Elf_Data *data = elf_getdata(scn, NULL);
  if (data == NULL) {
    return;
  }
  unsigned count = shdr.sh_size / shdr.sh_entsize;

  CoreSymbolInfoBuilder builder;

  for (unsigned i = 0; i < count; i++) {
    GElf_Sym sym;
    if (gelf_getsym(data, i, &sym) == NULL) {
      continue;
    }
    if (sym.st_shndx == SHN_ABS)
      continue;
    if (sym.st_value < low || sym.st_value >= high)
      continue;
    builder.addSymbol(elf_strptr(e, shdr.sh_link, sym.st_name),
                      sym.st_value,
                      sym.st_info);
  }
  SI = builder.getSymbolInfo();
}

static void readSymbols(Elf *e, unsigned low, unsigned high,
                        std::auto_ptr<CoreSymbolInfo> &SI)
{
  Elf_Scn *scn = NULL;
  GElf_Shdr shdr;
  while ((scn = elf_nextscn(e, scn)) != NULL) {
    if (gelf_getshdr(scn, &shdr) == NULL) {
      continue;
    }
    if (shdr.sh_type == SHT_SYMTAB) {
      // Found the symbol table
      break;
    }
  }
  
  if (scn != NULL) {
    readSymbols(e, scn, shdr, low, high, SI);
  }
}

static xmlNode *findChild(xmlNode *node, const char *name)
{
  for (xmlNode *child = node->children; child; child = child->next) {
    if (child->type != XML_ELEMENT_NODE)
      continue;
    if (strcmp(name, (char*)child->name) == 0)
      return child;
  }
  return 0;
}

static xmlAttr *findAttribute(xmlNode *node, const char *name)
{
  for (xmlAttr *child = node->properties; child; child = child->next) {
    if (child->type != XML_ATTRIBUTE_NODE)
      continue;
    if (strcmp(name, (char*)child->name) == 0)
      return child;
  }
  return 0;
}

static void readElf(const char *filename, const XEElfSector *elfSector,
                    Core &core, std::auto_ptr<CoreSymbolInfo> &SI,
                    std::map<Core*,uint32_t> &entryPoints)
{
  uint64_t ElfSize = elfSector->getElfSize();
  const scoped_array<char> buf(new char[ElfSize]);
  if (!elfSector->getElfData(buf.get())) {
    std::cerr << "Error reading elf data from \"" << filename << "\"" << std::endl;
    std::exit(1);
  }
  
  if (elf_version(EV_CURRENT) == EV_NONE) {
    std::cerr << "ELF library intialisation failed: "
              << elf_errmsg(-1) << std::endl;
    std::exit(1);
  }
  Elf *e;
  if ((e = elf_memory(buf.get(), ElfSize)) == NULL) {
    std::cerr << "Error reading ELF: " << elf_errmsg(-1) << std::endl;
    std::exit(1);
  }
  if (elf_kind(e) != ELF_K_ELF) {
    std::cerr << filename << " is not an ELF object" << std::endl;
    std::exit(1);
  }
  GElf_Ehdr ehdr;
  if (gelf_getehdr(e, &ehdr) == NULL) {
    std::cerr << "Reading ELF header failed: " << elf_errmsg(-1) << std::endl;
    std::exit(1);
  }
  if (ehdr.e_machine != XCORE_ELF_MACHINE) {
    std::cerr << "Not a XCore ELF" << std::endl;
    std::exit(1);
  }
  if (ehdr.e_entry != 0) {
    entryPoints.insert(std::make_pair(&core, (uint32_t)ehdr.e_entry));
  }
  unsigned num_phdrs = ehdr.e_phnum;
  if (num_phdrs == 0) {
    std::cerr << "No ELF program headers" << std::endl;
    std::exit(1);
  }
  uint32_t ram_base = core.ram_base;
  uint32_t ram_size = core.ram_size;
  for (unsigned i = 0; i < num_phdrs; i++) {
    GElf_Phdr phdr;
    if (gelf_getphdr(e, i, &phdr) == NULL) {
      std::cerr << "Reading ELF program header " << i << " failed: " << elf_errmsg(-1) << std::endl;
      std::exit(1);
    }
    if (phdr.p_filesz == 0) {
      continue;
    }
    if (phdr.p_offset > ElfSize) {
    	std::cerr << "Invalid offet in ELF program header" << i << std::endl;
    	std::exit(1);
    }
    uint32_t offset = phdr.p_paddr - core.ram_base;
    if (offset > ram_size || offset + phdr.p_filesz > ram_size || offset + phdr.p_memsz > ram_size) {
      std::cerr << "Error data from ELF program header " << i << " does not fit in memory" << std::endl;
      std::exit(1);
    }
    std::memcpy(core.mem() + offset, &buf[phdr.p_offset], phdr.p_filesz);
  }

  readSymbols(e, ram_base, ram_base + ram_size, SI);
  
  elf_end(e);
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

static uint32_t
exception(ThreadState &thread, Core &state, uint32_t pc, int et,
          uint32_t ed)
{
  uint32_t sed = thread.regs[ED];
  uint32_t spc = state.targetPc(pc);
  uint32_t ssr = thread.sr.to_ulong();
  
  if (Tracer::get().getTracingEnabled()) {
    Tracer::get().exception(thread, et, ed, sed, ssr, spc);
  }
  
  thread.regs[SSR] = sed;
  thread.regs[SPC] = spc;
  thread.regs[SED] = sed;
  thread.ink() = true;
  thread.eeble() = false;
  thread.ieble() = false;
  thread.regs[ET] = et;
  thread.regs[ED] = ed;

  uint32_t newPc = state.physicalAddress(thread.regs[KEP]);
  if (et == ET_KCALL)
    newPc += 64;
  if ((newPc & 1) || !state.isValidAddress(newPc)) {
    std::cout << "Error: unable to handle exception (invalid kep)\n";
    std::abort();
  }
  return newPc >> 1;
}

static void internalError(const ThreadState &thread, const char *file, int line) {
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
checkResource(ThreadState &thread, ResourceID id)
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
checkSync(ThreadState &thread, ResourceID id)
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
checkThread(ThreadState &thread, ResourceID id)
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
checkLock(ThreadState &thread, ResourceID id)
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
checkChanend(ThreadState &thread, ResourceID id)
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
checkTimer(ThreadState &thread, ResourceID id)
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
checkPort(ThreadState &thread, ResourceID id)
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
checkEventableResource(ThreadState &thread, ResourceID id)
{
  return checkEventableResource(thread.getParent(), id);
}

static ThreadState *
setSR(ThreadState &thread, SystemState &state, ThreadState::sr_t value)
{
  if (thread.setSR(value)) {
    state.takeEvent(thread, false);
  }
  return state.next(thread, thread.time);
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

static bool
setC(ThreadState &thread, ticks_t time, ResourceID resID, uint32_t val)
{
  Resource *res = thread.getParent().getResourceByID(resID);
  if (!res) {
    return false;
  }
  if (val == SETC_INUSE_OFF || val == SETC_INUSE_ON)
    return res->setCInUse(thread, val == SETC_INUSE_ON, time);
  if (!res->isInUse())
    return false;
  switch (val) {
  default:
    internalError(thread, __FILE__, __LINE__); // TODO
  case SETC_IE_MODE_EVENT:
  case SETC_IE_MODE_INTERRUPT:
    {
      if (!res->isEventable())
        return false;
      EventableResource *ER = static_cast<EventableResource *>(res);
      ER->setInterruptMode(thread, val == SETC_IE_MODE_INTERRUPT);
      break;
    }
  case SETC_COND_FULL:
  case SETC_COND_AFTER:
  case SETC_COND_EQ:
  case SETC_COND_NEQ:
    {
      if (!res->setCondition(thread, setCCondition(val), time))
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
        clock->start(thread, time);
      else
        clock->stop(thread, time);
      break;
    }
  case SETC_MS_MASTER:
  case SETC_MS_SLAVE:
    if (res->getType() != RES_TYPE_PORT)
      return false;
    return static_cast<Port*>(res)->setMasterSlave(thread, getMasterSlave(val),
                                                   time);
  case SETC_BUF_BUFFERS:
  case SETC_BUF_NOBUFFERS:
    if (res->getType() != RES_TYPE_PORT)
      return false;
    return static_cast<Port*>(res)->setBuffered(thread, val == SETC_BUF_BUFFERS,
                                                time);
  case SETC_RDY_NOREADY:
  case SETC_RDY_STROBED:
  case SETC_RDY_HANDSHAKE:
    if (res->getType() != RES_TYPE_PORT)
      return false;
    return static_cast<Port*>(res)->setReadyMode(thread, getPortReadyMode(val),
                                                 time);
  case SETC_PORT_DATAPORT:
  case SETC_PORT_CLOCKPORT:
  case SETC_PORT_READYPORT:
    if (res->getType() != RES_TYPE_PORT)
      return false;
    return static_cast<Port*>(res)->setPortType(thread, getPortType(val), time);
  case SETC_RUN_CLRBUF:
    {
      if (res->getType() != RES_TYPE_PORT)
        return false;
      static_cast<Port*>(res)->clearBuf(thread, time);
      break;
    }
  }
  return true;
}

const uint32_t CLK_REF = 0x1;

static bool
setClock(ThreadState &thread, ResourceID resID, uint32_t val, ticks_t time)
{
  Core &state = thread.getParent();
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
        c->setSourceRefClock(thread, time);
        return true;
      }
      Port *p = checkPort(state, val);
      if (!p || p->getPortWidth() != 1)
        return false;
      return c->setSource(thread, p, time);
    }
  case RES_TYPE_PORT:
    {
      Resource *source = state.getResourceByID(val);
      if (!source)
        return false;
      if (source->getType() != RES_TYPE_CLKBLK)
        return false;
      static_cast<Port*>(res)->setClk(thread, static_cast<ClockBlock*>(source), time);
      return true;
    }
    break;
  }
  return true;
}

static bool
setReady(ThreadState &thread, ResourceID resID, uint32_t val, ticks_t time)
{
  Core &state = thread.getParent();
  Resource *res = checkResource(state, resID);
  Port *ready = checkPort(state, val);
  if (!res || !ready) {
    return false;
  }
  return res->setReady(thread, ready, time);
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
  thread->pc = PC;\
} while(0)
#define UPDATE_CACHED() \
do { \
  PC = thread->pc;\
  core = &thread->getParent(); \
  opcode = core->opcode; \
  operands = core->operands; \
} while(0)
#define REG(Num) thread->regs[Num]
#define IMM(Num) (Num)
#define PC pc
#define TIME thread->time
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
  PC = exception(*thread, *core, PC, et, ed); \
  NEXT_THREAD(PC); \
} while(0);
#define ERROR() \
do { \
  SAVE_CACHED(); \
  internalError(*thread, __FILE__, __LINE__); \
} while(0)
#define TRACE(...) \
do { \
  if (tracing) { \
    SAVE_CACHED(); \
    Tracer::get().trace(*thread, __VA_ARGS__); \
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
  thread = sys.deschedule(*thread); \
  UPDATE_CACHED(); \
} while(0)
#define PAUSE_ON(pc, resource) \
do { \
  SAVE_CACHED(); \
  thread = sys.deschedule(*thread, resource); \
  UPDATE_CACHED(); \
} while(0)
#define NEXT_THREAD(pc) \
do { \
  if (sys.hasTimeSliceExpired(TIME)) { \
    SAVE_CACHED(); \
    thread = sys.next(*thread, thread->time); \
    UPDATE_CACHED(); \
  } \
} while(0)
#define TAKE_EVENT(pc) \
do { \
  SAVE_CACHED(); \
  thread = sys.takeEvent(*thread); \
  UPDATE_CACHED(); \
} while(0)
#define SETSR(bits, pc) \
do { \
  SAVE_CACHED(); \
  thread = setSR(*thread, sys, bits); \
  UPDATE_CACHED(); \
} while(0)

#define INSTRUCTION_CYCLES 4

typedef std::vector<std::pair<PortArg, PortArg> > LoopbackPorts;

static bool
connectLoopbackPorts(Core &state, const LoopbackPorts &ports)
{
  for (LoopbackPorts::const_iterator it = ports.begin(), e = ports.end();
       it != e; ++it) {
    Port *first = it->first.lookup(state);
    if (!first) {
      std::cerr << "Error: Invalid port ";
      it->first.dump(std::cerr);
      std::cerr << '\n';
      return false;
    }
    Port *second = it->second.lookup(state);
    if (!second) {
      std::cerr << "Error: Invalid port ";
      it->second.dump(std::cerr);
      std::cerr << '\n';
      return false;
    }
    first->setLoopback(second);
    second->setLoopback(first);
  }
  return true;
}

static void connectWaveformTracer(Core &core, WaveformTracer &waveformTracer)
{
  for (Core::port_iterator it = core.port_begin(), e = core.port_end();
       it != e; ++it) {
    waveformTracer.add(core.getCoreName(), *it);
  }
  waveformTracer.finalizePorts();
}

static void
connectWaveformTracer(SystemState &system, WaveformTracer &waveformTracer)
{
  for (SystemState::node_iterator outerIt = system.node_begin(),
       outerE = system.node_end(); outerIt != outerE; ++outerIt) {
    Node &node = **outerIt;
    for (Node::core_iterator innerIt = node.core_begin(),
         innerE = node.core_end(); innerIt != innerE; ++innerIt) {
      Core &core = **innerIt;
      connectWaveformTracer(core, waveformTracer);
    }
  }
}

static long readNumberAttribute(xmlNode *node, const char *name)
{
  xmlAttr *attr = findAttribute(node, name);
  assert(attr);
  errno = 0;
  long value = std::strtol((char*)attr->children->content, 0, 0);
  if (errno != 0) {
    std::cerr << "Invalid " << name << '"' << (char*)attr->children->content
              << "\"\n";
    exit(1);
  }
  return value;
}

static inline std::auto_ptr<Core>
createCoreFromConfig(xmlNode *config)
{
  uint32_t ram_size = RAM_SIZE;
  uint32_t ram_base = RAM_BASE;
  xmlNode *memoryController = findChild(config, "MemoryController");
  xmlNode *ram = findChild(memoryController, "Ram");
  ram_base = readNumberAttribute(ram, "base");
  ram_size = readNumberAttribute(ram, "size");
  std::auto_ptr<Core> core(new Core(ram_size, ram_base));
  core->setCoreNumber(readNumberAttribute(config, "number"));
  if (xmlAttr *codeReference = findAttribute(config, "codeReference")) {
    core->setCodeReference((char*)codeReference->children->content);
  }
  return core;
}

static inline std::auto_ptr<Node>
createNodeFromConfig(xmlNode *config,
                     std::map<long,Node*> &nodeNumberMap)
{
  long jtagID = readNumberAttribute(config, "jtagId");
  Node::Type nodeType;
  if (!Node::getTypeFromJtagID(jtagID, nodeType)) {
    std::cerr << "Unknown jtagId 0x" << std::hex << jtagID << std::dec << '\n';
    std::exit(1);
  }
  std::auto_ptr<Node> node(new Node(nodeType));
  long nodeID = readNumberAttribute(config, "number");
  nodeNumberMap.insert(std::make_pair(nodeID, node.get()));
  for (xmlNode *child = config->children; child; child = child->next) {
    if (child->type != XML_ELEMENT_NODE ||
        strcmp("Processor", (char*)child->name) != 0)
      continue;
    node->addCore(createCoreFromConfig(child));
  }
  node->setNodeID(nodeID);
  return node;
}

static inline std::auto_ptr<SystemState>
createSystemFromConfig(const char *filename, const XESector *configSector)
{
  uint64_t length = configSector->getLength();
  const scoped_array<char> buf(new char[length + 1]);
  if (!configSector->getData(buf.get())) {
    std::cerr << "Error reading config from \"" << filename << "\"" << std::endl;
    std::exit(1);
  }
  if (length < 8) {
    std::cerr << "Error unexpected config config sector length" << std::endl;
    std::exit(1);
  }
  length -= 8;
  buf[length] = '\0';
  
  /*
   * this initialize the library and check potential ABI mismatches
   * between the version it was compiled for and the actual shared
   * library used.
   */
  LIBXML_TEST_VERSION
  
  xmlDoc *doc = xmlReadDoc((xmlChar*)buf.get(), "config.xml", NULL, 0);

  xmlRelaxNGParserCtxtPtr schemaContext =
    xmlRelaxNGNewMemParserCtxt(configSchema, sizeof(configSchema));
  xmlRelaxNGPtr schema = xmlRelaxNGParse(schemaContext);
  xmlRelaxNGValidCtxtPtr validationContext =
    xmlRelaxNGNewValidCtxt(schema);
  if (xmlRelaxNGValidateDoc(validationContext, doc) != 0) {
    std::exit(1);
  }

  xmlNode *root = xmlDocGetRootElement(doc);
  xmlNode *system = findChild(root, "System");
  xmlNode *nodes = findChild(system, "Nodes");
  std::auto_ptr<SystemState> systemState(new SystemState);
  std::map<long,Node*> nodeNumberMap;
  for (xmlNode *child = nodes->children; child; child = child->next) {
    if (child->type != XML_ELEMENT_NODE ||
        strcmp("Node", (char*)child->name) != 0)
      continue;
    systemState->addNode(createNodeFromConfig(child, nodeNumberMap));
  }
  xmlNode *jtag = findChild(system, "JtagChain");
  unsigned jtagIndex = 0;
  for (xmlNode *child = jtag->children; child; child = child->next) {
    if (child->type != XML_ELEMENT_NODE ||
        strcmp("Node", (char*)child->name) != 0)
      continue;
    long nodeID = readNumberAttribute(child, "id");
    std::map<long,Node*>::iterator it = nodeNumberMap.find(nodeID);
    if (it == nodeNumberMap.end()) {
      std::cerr << "No node matching id " << nodeID << std::endl;
      std::exit(1);
    }
    it->second->setJtagIndex(jtagIndex++);
  }
  xmlFreeDoc(doc);
  xmlCleanupParser();
  return systemState;
}

static inline void
addToCoreMap(std::map<std::pair<unsigned, unsigned>,Core*> &coreMap,
             Node &node)
{
  unsigned jtagIndex = node.getJtagIndex();
  const std::vector<Core*> &cores = node.getCores();
  unsigned coreNum = 0;
  for (std::vector<Core*>::const_iterator it = cores.begin(), e = cores.end();
       it != e; ++it) {
    coreMap.insert(std::make_pair(std::make_pair(jtagIndex, coreNum), *it));
    coreNum++;
  }
}


static inline void
addToCoreMap(std::map<std::pair<unsigned, unsigned>,Core*> &coreMap,
             SystemState &system)
{
  for (SystemState::node_iterator it = system.node_begin(),
       e = system.node_end(); it != e; ++it) {
    addToCoreMap(coreMap, **it);
  }
}

static inline std::auto_ptr<SystemState>
readXE(const char *filename, SymbolInfo &SI,
       std::set<Core*> &coresWithImage, std::map<Core*,uint32_t> &entryPoints)
{
  // Load the file into memory.
  XE xe(filename);
  if (!xe) {
    std::cerr << "Error opening \"" << filename << "\"" << std::endl;
    std::exit(1);
  }
  // TODO handle XEs / XBs without a config sector.
  const XESector *configSector = xe.getConfigSector();
  if (!configSector) {
    std::cerr << "Error: No config file found in \"" << filename << "\"" << std::endl;
    std::exit(1);
  }
  std::auto_ptr<SystemState> system =
    createSystemFromConfig(filename, configSector);
  std::map<std::pair<unsigned, unsigned>,Core*> coreMap;
  addToCoreMap(coreMap, *system);
  for (std::vector<const XESector *>::const_reverse_iterator
       it = xe.getSectors().rbegin(), end = xe.getSectors().rend(); it != end;
       ++it) {
    switch((*it)->getType()) {
    case XESector::XE_SECTOR_ELF:
      {
        const XEElfSector *elfSector = static_cast<const XEElfSector*>(*it);
        unsigned jtagIndex = elfSector->getNode();
        unsigned coreNum = elfSector->getCore();
        Core *core = coreMap[std::make_pair(jtagIndex, coreNum)];
        if (!core) {
          std::cerr << "Error: cannot find node " << jtagIndex
                    << ", core " << coreNum << std::endl;
          std::exit(1);
        }
        if (coresWithImage.count(core))
          continue;
        std::auto_ptr<CoreSymbolInfo> CSI;
        readElf(filename, elfSector, *core, CSI, entryPoints);
        SI.add(core, CSI);
        coresWithImage.insert(core);
        break;
      }
    }
  }
  xe.close();
  return system;
}

typedef std::vector<std::pair<PeripheralDescriptor*, Properties> >
  PeripheralDescriptorWithPropertiesVector;

template <bool tracing> int
loop(const char *filename, const LoopbackPorts &loopbackPorts,
     const std::string &vcdFile,
     const PeripheralDescriptorWithPropertiesVector &peripherals)
{
  std::auto_ptr<SymbolInfo> SI(new SymbolInfo);
  std::set<Core*> coresWithImage;
  std::map<Core*,uint32_t> entryPoints;
  std::auto_ptr<SystemState> statePtr = readXE(filename, *SI, coresWithImage,
                                               entryPoints);
  SystemState &sys = *statePtr;

  // TODO update to handle multiple cores.
  if (!connectLoopbackPorts(**coresWithImage.begin(), loopbackPorts)) {
    std::exit(1);
  }

  for (PeripheralDescriptorWithPropertiesVector::const_iterator it = peripherals.begin(),
       e = peripherals.end(); it != e; ++it) {
    it->first->createInstance(sys, it->second);
  }

  std::auto_ptr<WaveformTracer> waveformTracer;
  // TODO update to handle multiple cores.
  if (!vcdFile.empty()) {
    waveformTracer.reset(new WaveformTracer(vcdFile));
    connectWaveformTracer(sys, *waveformTracer);
  }

  for (std::set<Core*>::iterator it = coresWithImage.begin(),
       e = coresWithImage.end(); it != e; ++it) {
    Core *core = *it;
    if (sys.getExecutingThread())
      sys.schedule(core->getThread(0).getState());
    else
      sys.setExecutingThread(core->getThread(0).getState());
    core->initCache(OPCODE(DECODE), OPCODE(ILLEGAL_PC),
                    OPCODE(ILLEGAL_PC_THREAD), OPCODE(NO_THREADS));

    // Patch in syscall instruction at the syscall address.
    if (const ElfSymbol *syscallSym = SI->getGlobalSymbol(core, "_DoSyscall")) {
      uint32_t SyscallPc = core->physicalAddress(syscallSym->value) >> 1;
      if (SyscallPc < core->ram_size << 1) {
        core->opcode[SyscallPc] = OPCODE(SYSCALL);
      } else {
        std::cout << "Warning: invalid _DoSyscall address "
        << std::hex << syscallSym->value << std::dec << "\n";
      }
    }
    // Patch in exception instruction at the exception address
    if (const ElfSymbol *doExceptionSym = SI->getGlobalSymbol(core, "_DoException")) {
      uint32_t ExceptionPc = core->physicalAddress(doExceptionSym->value) >> 1;
      if (ExceptionPc < core->ram_size << 1) {
        core->opcode[ExceptionPc] = OPCODE(EXCEPTION);
      } else {
        std::cout << "Warning: invalid _DoException address "
        << std::hex << doExceptionSym->value << std::dec << "\n";
      }
    }
    std::map<Core*,uint32_t>::iterator match;
    if ((match = entryPoints.find(core)) != entryPoints.end()) {
      uint32_t entryPc = core->physicalAddress(match->second) >> 1;
      if (entryPc< core->ram_size << 1) {
        core->getThread(0).getState().pc = entryPc;
      } else {
        std::cout << "Warning: invalid ELF entry point 0x";
        std::cout << std::hex << match->second << std::dec << "\n";
      }
    }
  }
  SyscallHandler::setCoreCount(coresWithImage.size());
  
  // Initialise tracing
  Tracer::get().setSymbolInfo(SI);
  if (tracing) {
    Tracer::get().setTracingEnabled(tracing);
  }

  ThreadState *thread = statePtr->getExecutingThread();
  uint32_t pc = thread->pc;
  Core *core = &thread->getParent();
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
      Synchroniser *sync = thread->getSync();
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
      Synchroniser *sync = thread->getSync();
      // May schedule / deschedule threads
      if (!sync) {
        // TODO is this right?
        DESCHEDULE(PC);
      } else {
        switch (sync->ssync(*thread)) {
        case Synchroniser::SYNC_CONTINUE:
          PC++;
          break;
        case Synchroniser::SYNC_DESCHEDULE:
          PAUSE_ON(PC, sync);
          break;
        case Synchroniser::SYNC_KILL:
          {
            ThreadState *slave = thread;
            DESCHEDULE(PC);
            slave->getRes().free();
          }
        }
      }
      TRACE_END();
    }
    ENDINST;
  INST(FREET_0r):
    {
      TRACE("freet");
      if (thread->getSync()) {
        // TODO check expected behaviour
        ERROR();
      }
      ThreadState *t = thread;
      DESCHEDULE(PC);
      t->getRes().free();
      TRACE_END();
    }
    ENDINST;
  
  // Pseudo instructions.
  INST(SYSCALL):
    int retval;
    thread->pc = PC;
    switch (SyscallHandler::doSyscall(*thread, retval)) {
    case SyscallHandler::EXIT:
      return retval;
      break;
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
    thread->pc = PC;
    SyscallHandler::doException(*thread);
    return 1;
    ENDINST;
  INST(ILLEGAL_PC):
    EXCEPTION(ET_ILLEGAL_PC, FROM_PC(PC));
    ENDINST;
  INST(ILLEGAL_PC_THREAD):
    EXCEPTION(ET_ILLEGAL_PC, thread->illegal_pc);
    ENDINST;
  INST(NO_THREADS):
    return 1;
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

static void loopbackOption(const char *a, const char *b, LoopbackPorts &loopbackPorts)
{
  PortArg firstArg;
  if (!PortArg::parse(a, firstArg)) {
    std::cerr << "Error: Invalid port " << a << '\n';
    exit(1);
  }
  PortArg secondArg;
  if (!PortArg::parse(b, secondArg)) {
    std::cerr << "Error: Invalid port " << b << '\n';
    exit(1);
  }
  loopbackPorts.push_back(std::make_pair(firstArg, secondArg));
}

static Property
parseIntegerProperty(const PropertyDescriptor *prop, const std::string &s)
{
  char *endp;
  long value = std::strtol(s.c_str(), &endp, 0);
  if (*endp != '\0') {
    std::cerr << "Error: property " << prop->getName();
    std::cerr << " requires an integer argument\n";
    std::exit(1);
  }
  return Property::integerProperty(prop, value);
}

static Property
parsePortProperty(const PropertyDescriptor *prop, const std::string &s)
{
  PortArg portArg;
  if (!PortArg::parse(s, portArg)) {
    std::cerr << "Error: property " << prop->getName();
    std::cerr << " requires an port argument\n";
    std::exit(1);
  }
  return Property::portProperty(prop, portArg);
}

static void
parseProperties(const std::string &str, const PeripheralDescriptor *periph,
                Properties &properties)
{
  //port=PORT_1A,bitrate=28800
  const char *s = str.c_str();
  while (*s != '\0') {
    const char *p = std::strpbrk(s, "=,");
    std::string name;
    std::string value;
    if (!p) {
      name = std::string(s);
      s += name.length();
    } else {
      name = std::string(s, p - s);
      s = p + 1;
      if (*p == '=') {
        p = strchr(s, ',');
        if (p) {
          value = std::string(s, p - s);
          s += value.length() + 1;
        } else {
          value = std::string(s);
          s += value.length();
        }
      }
    }
    if (const PropertyDescriptor *prop = periph->getProperty(name)) {
      switch (prop->getType()) {
      default:
        assert(0 && "Unexpected property type");
      case PropertyDescriptor::INTEGER:
        properties.set(parseIntegerProperty(prop, value));
        break;
      case PropertyDescriptor::STRING:
        properties.set(Property::stringProperty(prop, value));
        break;
      case PropertyDescriptor::PORT:
        properties.set(parsePortProperty(prop, value));
        break;
      }
    } else {
      std::cerr << "Error: Unknown property \"" << name << "\"";
      std::cerr << " for " << periph->getName() << std::endl;
      std::exit(1);
    }
  }
  // Check required properties have been set.
  for (PeripheralDescriptor::const_iterator it = periph->properties_begin(),
       e = periph->properties_end(); it != e; ++it) {
    if (it->getRequired() && !properties.get(it->getName())) {
      std::cerr << "Error: Required property \"" << it->getName() << "\"";
      std::cerr << " for " << periph->getName();
      std::cerr << " is not set " << std::endl;
      std::exit(1);
    }
  }
}

static PeripheralDescriptor *parsePeripheralOption(const std::string arg)
{
  if (arg.substr(0, 2) != "--")
    return 0;
  return PeripheralRegistry::get(arg.substr(2));
}

int
main(int argc, char **argv) {
  registerAllPeripherals();
  if (argc < 2) {
    printUsage(argv[0]);
    return 1;
  }
  const char *file = 0;
  bool tracing = false;
  LoopbackPorts loopbackPorts;
  std::string vcdFile;
  std::string arg;
  std::vector<std::pair<PeripheralDescriptor*, Properties> > peripherals;
  for (int i = 1; i < argc; i++) {
    arg = argv[i];
    if (arg == "-t") {
      tracing = true;
    } else if (arg == "--vcd") {
      if (i + 1 > argc) {
        printUsage(argv[0]);
        return 1;
      }
      vcdFile = argv[i + 1];
      i++;
    } else if (arg == "--loopback") {
      if (i + 2 >= argc) {
        printUsage(argv[0]);
        return 1;
      }
      loopbackOption(argv[i + 1], argv[i + 2], loopbackPorts);
      i += 2;
    } else if (arg == "--help") {
      printUsage(argv[0]);
      return 0;
    } else if (PeripheralDescriptor *pd = parsePeripheralOption(arg)) {
      peripherals.push_back(std::make_pair(pd, Properties()));
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        parseProperties(argv[++i], pd, peripherals.back().second);
      }
    } else {
      if (file) {
        printUsage(argv[0]);
        return 1;
      }
      file = argv[i];
    }
  }
  if (!file) {
    printUsage(argv[0]);
    return 1;
  }
#ifndef _WIN32
  if (isatty(fileno(stdout))) {
    Tracer::get().setColour(true);
  }
#endif
  if (tracing) {
    return loop<true>(file, loopbackPorts, vcdFile, peripherals);
  } else {
    return loop<false>(file, loopbackPorts, vcdFile, peripherals);
  }
}
