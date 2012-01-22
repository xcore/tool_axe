// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Core_h_
#define _Core_h_

#include <stdint.h>
#include <ostream>
#include <algorithm>
#include <iterator>
#include <cstring>
#include <cstdlib>
#include "BitManip.h"
#include "Config.h"
#include "Resource.h"
#include "Lock.h"
#include "Synchroniser.h"
#include "Chanend.h"
#include "ClockBlock.h"
#include "Port.h"
#include "Thread.h"
#include "Timer.h"
#include "Instruction.h"
#include "Trace.h"
#include "RunnableQueue.h"
#include <string>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

class Node;

enum ProcessorState {
  PS_RAM_BASE = 0x00b,
  PS_VECTOR_BASE = 0x10b
};

class Core {
public:
  enum {
    ILLEGAL_PC_THREAD_ADDR_OFFSET = 2,
  };
  enum {
    INVALIDATE_NONE,
    INVALIDATE_CURRENT,
    INVALIDATE_CURRENT_AND_PREVIOUS
  };
private:
  Thread * const thread;
  Synchroniser * const sync;
  Lock * const lock;
  Chanend * const chanend;
  Timer * const timer;
  ClockBlock * const clkBlk;
  Port ** const port;
  unsigned *portNum;
  Resource ***resource;
  unsigned *resourceNum;
  static bool allocatable[LAST_STD_RES_TYPE + 1];
  uint32_t * const memory;
  unsigned coreNumber;
  Node *parent;
  std::string codeReference;
  OPCODE_TYPE decodeOpcode;
  OPCODE_TYPE jitFunctionOpcode;

  bool hasMatchingNodeID(ResourceID ID);
  void invalidateWordSlowPath(uint32_t address);
  void invalidateSlowPath(uint32_t shiftedAddress);

  bool invalidateWord(uint32_t address) {
    if (invalidationInfo[address >> 1] == INVALIDATE_NONE &&
        invalidationInfo[(address >> 1) + 1] == INVALIDATE_NONE)
      return false;
    invalidateWordSlowPath(address);
    return true;
  }

  bool invalidateShort(uint32_t address) {
    if (invalidationInfo[address >> 1] == INVALIDATE_NONE)
      return false;
    invalidateSlowPath(address >> 1);
    return true;
  }

  bool invalidateByte(uint32_t address) {
    if (invalidationInfo[address >> 1] == INVALIDATE_NONE)
      return false;
    invalidateSlowPath(address >> 1);
    return true;
  }
public:
  // The opcode cache is bigger than the memory size. We place an ILLEGAL_PC
  // pseudo instruction just past the end of memory. This saves
  // us from having to check for illegal pc values when incrementing the pc from
  // the previous instruction. Addition pseudo instructions come after this and
  // are use for communicating illegal states.
  OPCODE_TYPE *opcode;
  Operands *operands;
  // TODO merge these arrays.
  unsigned char *invalidationInfo;

  const uint32_t ram_size;
  const uint32_t ram_base;
  uint32_t vector_base;

  uint32_t syscallAddress;
  uint32_t exceptionAddress;

  Core(uint32_t RamSize, uint32_t RamBase) :
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
    memory(new uint32_t[RamSize >> 2]),
    coreNumber(0),
    parent(0),
    opcode(new OPCODE_TYPE[(RamSize >> 1) + ILLEGAL_PC_THREAD_ADDR_OFFSET]),
    operands(new Operands[RamSize >> 1]),
    invalidationInfo(new unsigned char[RamSize >> 1]),
    ram_size(RamSize),
    ram_base(RamBase),
    syscallAddress(~0),
    exceptionAddress(~0)
  {
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
    for (unsigned i = 0; i < ARRAY_SIZE(portSpec); i++) {
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

    // Initialise instruction cache.
    for (unsigned i = 0; i < (RamSize >> 1) + ILLEGAL_PC_THREAD_ADDR_OFFSET;
         ++i) {
#ifdef DIRECT_THREADED
      opcode[i] = 0;
#else
      opcode[i] = INITIALIZE;
#endif
    }
    for (unsigned i = 0; i != (RamSize >> 1); ++i) {
      invalidationInfo[i] = INVALIDATE_NONE;
    }
  }

  bool setSyscallAddress(uint32_t value);
  bool setExceptionAddress(uint32_t value);

  void initCache(OPCODE_TYPE decode, OPCODE_TYPE illegalPC,
                 OPCODE_TYPE illegalPCThread, OPCODE_TYPE syscall,
                 OPCODE_TYPE exception, OPCODE_TYPE jitFunction);

  ~Core() {
    delete[] opcode;
    delete[] operands;
    delete[] invalidationInfo;
    //delete[] thread;
    //delete[] sync;
    //delete[] lock;
    //delete[] chanend;
    //delete[] timer;
    //delete[] resource;
    //delete[] resourceNum;
    delete[] memory;
  }
  
  uint32_t targetPc(unsigned pc) const
  {
    return ram_base + (pc << 1);
  }

  uint32_t virtualAddress(uint32_t address) const
  {
    return address + ram_base;
  }

  uint32_t physicalAddress(uint32_t address) const
  {
    return address - ram_base;
  }
  
  bool isValidAddress(uint32_t address) const
  {
    return address < ram_size;
  }

  uint8_t *mem() {
    return reinterpret_cast<uint8_t*>(memory);
  }

  const uint8_t *mem() const {
    return reinterpret_cast<uint8_t*>(memory);
  }

  uint32_t loadWord(uint32_t address) const
  {
    if (HOST_LITTLE_ENDIAN) {
      return *reinterpret_cast<const uint32_t*>((mem() + address));
    } else {
      return bswap32(*reinterpret_cast<const uint32_t*>((mem() + address)));
    }
  }

  int16_t loadShort(uint32_t address) const
  {
    return loadByte(address) | loadByte(address + 1) << 8;
  }

  uint8_t loadByte(uint32_t address) const
  {
    return mem()[address];
  }

  uint8_t &byte(uint32_t address)
  {
    return mem()[address];
  }

  bool storeWord(uint32_t value, uint32_t address)
  {
    if (HOST_LITTLE_ENDIAN) {
      *reinterpret_cast<uint32_t*>((mem() + address)) = value;
    } else {
      *reinterpret_cast<uint32_t*>((mem() + address)) = bswap32(value);
    }
    return invalidateWord(address);
  }

  bool storeShort(int16_t value, uint32_t address)
  {
    mem()[address] = static_cast<uint8_t>(value);
    mem()[address + 1] = static_cast<uint8_t>(value >> 8);
    return invalidateShort(address);
  }

  bool storeByte(uint8_t value, uint32_t address)
  {
    mem()[address] = value;
    return invalidateByte(address);
  }

  Resource *allocResource(Thread &current, ResourceType type)
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

  Thread *allocThread(Thread &current)
  {
    return static_cast<Thread*>(allocResource(current, RES_TYPE_THREAD));
  }

  const Port *getPortByID(ResourceID ID) const;

  /// Returns the resource associated with the resource ID or NULL if the
  /// the resource ID is invalid.
  Resource *getResourceByID(ResourceID ID);
  const Resource *getResourceByID(ResourceID ID) const;

  bool getLocalChanendDest(ResourceID ID, ChanEndpoint *&result);
  ChanEndpoint *getChanendDest(ResourceID ID);

  unsigned getIllegalPCThreadAddr() const
  {
    return ((ram_size >> 1) - 1) + ILLEGAL_PC_THREAD_ADDR_OFFSET;
  }

  void updateIDs();

  /// Set the parent of the current core. updateIDs() must be called to update
  /// The core IDs and the channel end resource IDs.
  void setParent(Node *n) {
    parent = n;
  }
  void setCoreNumber(unsigned value) { coreNumber = value; }
  unsigned getCoreNumber() const { return coreNumber; }
  uint32_t getCoreID() const;
  const Node *getParent() const { return parent; }
  Node *getParent() { return parent; }
  void dumpPaused() const;
  Thread &getThread(unsigned num) { return thread[num]; }
  const Thread &getThread(unsigned num) const { return thread[num]; }
  void setCodeReference(const std::string &value) { codeReference = value; }
  const std::string &getCodeReference() const { return codeReference; }
  std::string getCoreName() const;

  class port_iterator {
    Core *core;
    unsigned width;
    unsigned num;
  public:
    port_iterator(Core *c, unsigned w, unsigned n) :
      core(c), width(w), num(n) {}
    const port_iterator &operator++() {
      ++num;
      if (num >= core->portNum[width]) {
        num = 0;
        do {
          ++width;
        } while (width != 33 && num >= core->portNum[width]);
      }
      return *this;
    }
    port_iterator operator++(int) {
      port_iterator it(*this);
      ++(*this);
      return it;
    }
    bool operator==(const port_iterator &other) {
      return other.width == width &&
      other.num == num;
    }
    bool operator!=(const port_iterator &other) {
      return !(*this == other);
    }
    Port *operator*() {
      return &core->port[width][num];
    }
  };
  port_iterator port_begin() { return port_iterator(this, 1, 0); }
  port_iterator port_end() { return port_iterator(this, 33, 0); }
};

#endif // _Core_h_
