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
#include "Thread.h"
#include "Port.h"
#include "Instruction.h"
#include <string>
#include <climits>

class Lock;
class Synchroniser;
class Chanend;
class ChanEndpoint;
class ClockBlock;
class Port;
class Timer;

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

class Node;

enum ProcessorState {
  PS_RAM_BASE = 0x00b,
  PS_VECTOR_BASE = 0x10b
};

class Core {
public:
  enum {
    YIELD_ADDR_OFFSET = 2,
    ILLEGAL_PC_THREAD_ADDR_OFFSET = 3
  };
  enum {
    INVALIDATE_NONE,
    INVALIDATE_CURRENT,
    INVALIDATE_CURRENT_AND_PREVIOUS
  };
private:
  typedef int executionFrequency_t;
  static const executionFrequency_t MIN_EXECUTION_FREQUENCY = INT_MIN;

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

  void runJIT(uint32_t shiftedAddress);
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
  executionFrequency_t *executionFrequency;

  const uint32_t ram_size;
  const uint32_t ram_base;
  uint32_t vector_base;

  uint32_t syscallAddress;
  uint32_t exceptionAddress;

  Core(uint32_t RamSize, uint32_t RamBase);
  ~Core();

  bool setSyscallAddress(uint32_t value);
  bool setExceptionAddress(uint32_t value);

  void initCache(OPCODE_TYPE decode, OPCODE_TYPE illegalPC,
                 OPCODE_TYPE illegalPCThread, OPCODE_TYPE yield, 
                 OPCODE_TYPE syscall, OPCODE_TYPE exception,
                 OPCODE_TYPE jitFunction);

  void updateExecutionFrequency(uint32_t shiftedAddress) {
    const executionFrequency_t threshold = 20;
    if (++executionFrequency[shiftedAddress] > threshold) {
      runJIT(shiftedAddress);
    }
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

  unsigned getIllegalPCThreadAddr() const {
    return ((ram_size >> 1) - 1) + ILLEGAL_PC_THREAD_ADDR_OFFSET;
  }
  
  unsigned getYieldAddr() const {
    return ((ram_size >> 1) - 1) + YIELD_ADDR_OFFSET;
  }

  OPCODE_TYPE getJitFunctionOpcode() const { return jitFunctionOpcode; }

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
