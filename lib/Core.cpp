// Copyright (c) 2011-12, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Core.h"
#include "Array.h"
#include "SystemState.h"
#include "ProcessorNode.h"
#include "JIT.h"
#include "Timer.h"
#include "Synchroniser.h"
#include "Lock.h"
#include "Chanend.h"
#include "ClockBlock.h"
#include "Tracer.h"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace axe;

Core::Core(uint32_t RamSize, uint32_t RamBase, bool tracing) :
  ramDecodeCache(RamBase >> 1, RamBase, true, tracing),
  ramSizeLog2(31 - countLeadingZeros(RamSize)),
  ramBaseMultiple(RamBase / RamSize),
  thread(new Thread[NUM_THREADS]),
  sync(new Synchroniser[NUM_SYNCS]),
  lock(new Lock[NUM_LOCKS]),
  chanend(new Chanend[NUM_CHANENDS]),
  timer(new Timer[NUM_TIMERS]),
  clkBlk(new ClockBlock[NUM_CLKBLKS]),
  port(new Port*[33]),
  portNum(new unsigned[33]),
  resource(new Resource**[LAST_STD_RES_TYPE + 1]),
  resourceNum(new unsigned[LAST_STD_RES_TYPE + 1]),
  memory(new uint8_t[RamSize]),
  coreNumber(0),
  parent(0),
  bootConfig(0),
  bootStatus(0),
  rom(0),
  romBase(0),
  romSize(0)
{
  memoryOffset = memory - RamBase;
  invalidationInfoOffset =
    ramDecodeCache.getState().getInvalidationInfo() - (RamBase / 2);

  resource[RES_TYPE_PORT] = 0;
  resourceNum[RES_TYPE_PORT] = 0;
  
  resource[RES_TYPE_TIMER] = new Resource*[NUM_TIMERS];
  for (unsigned i = 0; i < NUM_TIMERS; i++) {
    resource[RES_TYPE_TIMER][i] = &timer[i];
  }
  resourceNum[RES_TYPE_TIMER] = NUM_TIMERS;
  
  resource[RES_TYPE_CHANEND] = new Resource*[NUM_CHANENDS];
  for (unsigned i = 0; i < NUM_CHANENDS; i++) {
    resource[RES_TYPE_CHANEND][i] = &chanend[i];
  }
  resourceNum[RES_TYPE_CHANEND] = NUM_CHANENDS;
  
  resource[RES_TYPE_SYNC] = new Resource*[NUM_SYNCS];
  for (unsigned i = 0; i < NUM_SYNCS; i++) {
    resource[RES_TYPE_SYNC][i] = &sync[i];
  }
  resourceNum[RES_TYPE_SYNC] = NUM_SYNCS;
  
  resource[RES_TYPE_THREAD] = new Resource*[NUM_THREADS];
  for (unsigned i = 0; i < NUM_THREADS; i++) {
    thread[i].setParent(*this);
    resource[RES_TYPE_THREAD][i] = &thread[i];
  }
  resourceNum[RES_TYPE_THREAD] = NUM_THREADS;
  
  resource[RES_TYPE_LOCK] = new Resource*[NUM_LOCKS];
  for (unsigned i = 0; i < NUM_LOCKS; i++) {
    resource[RES_TYPE_LOCK][i] = &lock[i];
  }
  resourceNum[RES_TYPE_LOCK] = NUM_LOCKS;
  
  resource[RES_TYPE_CLKBLK] = new Resource*[NUM_CLKBLKS];
  for (unsigned i = 0; i < RES_TYPE_CLKBLK; i++) {
    resource[RES_TYPE_CLKBLK][i] = &clkBlk[i];
  }
  resourceNum[RES_TYPE_CLKBLK] = NUM_CLKBLKS;
  
  for (int i = RES_TYPE_TIMER; i <= LAST_STD_RES_TYPE; i++) {
    for (unsigned j = 0; j < resourceNum[i]; j++) {
      resource[i][j]->setNum(j);
    }
  }
  
  std::memset(port, 0, sizeof(port[0]) * 33);
  std::memset(portNum, 0, sizeof(portNum[0]) * 33);
  const unsigned portSpec[][2] = {
    {1, NUM_1BIT_PORTS},
    {4, NUM_4BIT_PORTS},
    {8, NUM_8BIT_PORTS},
    {16, NUM_16BIT_PORTS},
    {32, NUM_32BIT_PORTS},
  };
  for (unsigned i = 0; i < arraySize(portSpec); i++) {
    unsigned width = portSpec[i][0];
    unsigned num = portSpec[i][1];
    port[width] = new Port[num];
    for (unsigned j = 0; j < num; j++) {
      port[width][j].setNum(j);
      port[width][j].setWidth(width);
      port[width][j].setClkInitial(&clkBlk[0]);
    }
    portNum[width] = num;
  }
  thread[0].alloc(0);
}

Core::~Core() {
  delete[] thread;
  delete[] sync;
  delete[] lock;
  delete[] chanend;
  delete[] timer;
  delete[] resource;
  delete[] resourceNum;
  delete[] memory;
}

bool Core::allocatable[LAST_STD_RES_TYPE + 1] = {
  false, // RES_TYPE_PORT
  true, // RES_TYPE_TIMER
  true, // RES_TYPE_CHANEND
  true, // RES_TYPE_SYNC
  true, // RES_TYPE_THREAD
  true, // RES_TYPE_LOCK
  false, // RES_TYPE_CLKBLK
};

void Core::setRamBaseMultiple(unsigned multiple)
{
  // Invalidate cache.
  resetCaches();

  // Update ram base.
  ramBaseMultiple = multiple;
  uint32_t ramBase = getRamBase();
  ramDecodeCache.getState().setBase(ramBase);
  memoryOffset = memory - ramBase;
  invalidationInfoOffset =
    ramDecodeCache.getState().getInvalidationInfo() - (ramBase / 2);

  // Inform threads of the change.
  for (unsigned i = 0; i < NUM_THREADS; i++) {
    getThread(i).seeRamDecodeCacheChange();
  }
}

Tracer *Core::getTracer()
{
  return parent->getParent()->getTracer();
}

void Core::dumpPaused() const
{
  for (unsigned i = 0; i < NUM_THREADS; i++) {
    Thread *t = static_cast<Thread*>(resource[RES_TYPE_THREAD][i]);
    if (!t->isInUse())
      continue;
    std::cout << "Thread " << std::dec << i;
    Thread &ts = *t;
    if (Resource *res = ts.pausedOn) {
      std::cout << " paused on ";
      std::cout << Resource::getResourceName(res->getType());
      std::cout << " 0x" << std::hex << res->getID();
    } else if (ts.eeble()) {
      std::cout << " waiting for events";
      if (ts.ieble())
        std::cout << " or interrupts";
    } else if (ts.ieble()) {
      std::cout << " waiting for interrupts";
    } else {
      std::cout << " paused on unknown";
    }
    std::cout << "\n";
  }
}

Resource *Core::allocResource(Thread &current, ResourceType type)
{
  if (type > LAST_STD_RES_TYPE || !allocatable[type])
    return 0;
  for (unsigned i = 0; i < resourceNum[type]; i++) {
    if (!resource[type][i]->isInUse()) {
      bool allocated = resource[type][i]->alloc(current);
      assert(allocated);
      (void)allocated; // Silence compiler.
      return resource[type][i];
    }
  }
  return 0;
}

bool Core::readMemory(uint32_t address, void *dst, size_t size)
{
  if (size == 0)
    return true;
  uint32_t ramEnd = getRamBase() + getRamSize();
  if (address < getRamBase() || address + size >= ramEnd)
    return false;
  std::memcpy(dst, &memOffset()[address], size);
  return true;
}

bool Core::writeMemory(uint32_t address, const void *src, size_t size)
{
  if (size == 0)
    return true;
  uint32_t ramEnd = getRamBase() + getRamSize();
  if (address < getRamBase() || address + size >= ramEnd)
    return false;
  std::memcpy(&memOffset()[address], src, size);
  invalidateRange(address, address + size);
  return true;
}

const Port *Core::getPortByID(ResourceID ID) const
{
  assert(ID.type() == RES_TYPE_PORT);
  unsigned width = ID.width();
  if (width > 32)
    return 0;
  unsigned num = ID.num();
  if (num >= portNum[width])
    return 0;
  return &port[width][num];
}

const Resource *Core::getResourceByID(ResourceID ID) const
{
  ResourceType type = ID.type();
  if (type > LAST_STD_RES_TYPE) {
    return 0;
  }
  if (type == RES_TYPE_PORT) {
    return getPortByID(ID);
  }
  unsigned num = ID.num();
  if (num >= resourceNum[type]) {
    return 0;
  }
  return resource[type][num];
}

Resource *Core::getResourceByID(ResourceID ID)
{
  return const_cast<Resource *>(
    static_cast<const Core *>(this)->getResourceByID(ID)
  );
}

const DecodeCache::State *Core::
getDecodeCacheContaining(uint32_t address) const
{
  if (isValidRamAddress(address))
    return &getRamDecodeCache();
  if (isValidRomAddress(address))
    return &getParent()->getParent()->getRomDecodeCache();
  return 0;
}

bool Core::setBreakpoint(uint32_t value)
{
  if ((value & 1) || !isValidAddress(value))
    return false;
  if (breakpoints.insert(value).second)
    invalidateShort(value);
  return true;
}

void Core::unsetBreakpoint(uint32_t value)
{
  if (breakpoints.erase(value))
    invalidateShort(value);
}

void Core::resetCaches()
{
  uint32_t ramEnd = getRamBase() + (1 << ramSizeLog2);
  for (unsigned address = getRamBase(); address < ramEnd; address += 4) {
    invalidateWord(address);    
  }
}

bool Core::getLocalChanendDest(ResourceID ID, ChanEndpoint *&result)
{
  assert(ID.isChanendOrConfig());
  if (ID.isConfig()) {
    switch (ID.num()) {
    case RES_CONFIG_SSCTRL:
      return false;
    case RES_CONFIG_PSCTRL:
      // TODO.
      assert(0);
      result = 0;
      return true;
    }
  } else {
    if (ID.node() == getCoreID()) {
      Resource *res = getResourceByID(ID);
      if (res) {
        assert(res->getType() == RES_TYPE_CHANEND);
        result = static_cast<Chanend*>(res);
      } else {
        result = 0;
      }
      return true;
    }
  }
  return false;
}

ChanEndpoint *Core::getChanendDest(ResourceID ID)
{
  if (!ID.isChanendOrConfig())
    return 0;
  ChanEndpoint *result;
  // Try to lookup locally first.
  if (getLocalChanendDest(ID, result))
    return result;
  return parent->getOutgoingChanendDest(ID);
}

bool Core::setProcessorState(uint32_t reg, uint32_t value)
{
  switch (reg) {
  case PS_VECTOR_BASE:
    vector_base = value;
    return true;
  case PS_RAM_BASE:
    setRamBaseMultiple(value / getRamSize());
    return true;
  case PS_BOOT_STATUS:
    bootStatus = value;
    return true;
  }
  return false;
}

bool Core::getProcessorState(uint32_t reg, uint32_t &value)
{
  switch (reg) {
  case PS_VECTOR_BASE:
    value = vector_base;
    return true;
  case PS_RAM_BASE:
    value = getRamBase();
    return true;
  case PS_BOOT_CONFIG:
    value = bootConfig;
    return true;
  case PS_BOOT_STATUS:
    value = bootStatus;
    return true;
  }
  return false;
}

void Core::finalize()
{
  for (unsigned i = 0; i < NUM_THREADS; i++) {
    thread[i].finalize();
  }
}

void Core::updateIDs()
{
  unsigned coreID = getCoreID();
  for (unsigned i = 0; i < NUM_CHANENDS; i++) {
    resource[RES_TYPE_CHANEND][i]->setNode(coreID);
  }    
}

uint32_t Core::getCoreID() const
{
  return getParent()->getCoreID(coreNumber);
}

std::string Core::getCoreName() const
{
  if (!codeReference.empty())
    return codeReference;
  std::ostringstream buf;
  buf << 'c' << getCoreID();
  return buf.str();
}

void Core::invalidateWordSlowPath(uint32_t shiftedAddress)
{
  if (invalidationInfoOffset[shiftedAddress + 1] == DecodeCache::INVALIDATE_NONE) {
    invalidateSlowPath(shiftedAddress);
    return;
  }
  invalidationInfoOffset[shiftedAddress + 1] = DecodeCache::INVALIDATE_CURRENT_AND_PREVIOUS;
  invalidateSlowPath(shiftedAddress + 1);
}

void Core::invalidateSlowPath(uint32_t shiftedAddress)
{
  unsigned char info;
  JIT &jit = getParent()->getParent()->getJIT();
  do {
    info = invalidationInfoOffset[shiftedAddress];
    uint32_t pc = shiftedAddress - (getRamBase()/2);
    if (!jit.invalidate(*this, pc))
      clearOpcode(pc);
    ramDecodeCache.getState().executionFrequency[pc] = 0;
    invalidationInfoOffset[shiftedAddress--] = DecodeCache::INVALIDATE_NONE;
  } while (info == DecodeCache::INVALIDATE_CURRENT_AND_PREVIOUS);
}

bool Core::invalidateRange(uint32_t begin, uint32_t end)
{
  uint32_t address = begin;
  bool invalidated = false;
  for (; address % 4 && address < end; ++address) {
    invalidated |= invalidateByte(address);
  }
  for (; address + 3 < end; address += 4) {
    invalidated |= invalidateWord(address);
  }
  for (; address < end; ++address) {
    invalidated |= invalidateByte(address);
  }
  return invalidated;
}

void Core::runJIT(uint32_t jitPc)
{
  if (!isValidRamPc(jitPc))
    return;
  ramDecodeCache.getState().executionFrequency[jitPc] =
    DecodeCache::MIN_EXECUTION_FREQUENCY;
  getParent()->getParent()->getJIT().compileBlock(*this, jitPc);
}

void Core::clearOpcode(uint32_t pc)
{
  ramDecodeCache.getState().clearOpcode(pc);
}

void Core::setOpcode(uint32_t pc, OPCODE_TYPE opc, unsigned size)
{
  ramDecodeCache.getState().setOpcode(pc, opc, size);
}

void Core::setOpcode(uint32_t pc, OPCODE_TYPE opc, Operands &ops, unsigned size)
{
  ramDecodeCache.getState().setOpcode(pc, opc, ops, size);
}

void Core::setRom(const uint8_t *data, uint32_t base, uint32_t size)
{
  rom = data;
  romBase = base;
  romSize = size;
}
