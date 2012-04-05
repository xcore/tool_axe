// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Core_h_
#define _Core_h_

#include "Config.h"
#include <iterator>
#include <cstring>
#include "Resource.h"
#include "Thread.h"
#include "Port.h"
#include "Instruction.h"
#include "BitManip.h"
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
    RUN_JIT_ADDR_OFFSET = 2,
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
  uint32_t * memoryOffset;
  unsigned coreNumber;
  Node *parent;
  std::string codeReference;
  OPCODE_TYPE decodeOpcode;

  bool hasMatchingNodeID(ResourceID ID);
  void invalidateWordSlowPath(uint32_t address);
  void invalidateSlowPath(uint32_t shiftedAddress);

  bool invalidateWord(uint32_t address) {
    uint16_t info;
    std::memcpy(&info, &invalidationInfoOffset[address >> 1], sizeof(info));
    if (info == (INVALIDATE_NONE | (INVALIDATE_NONE << 8)))
      return false;
    invalidateWordSlowPath(address);
    return true;
  }

  bool invalidateShort(uint32_t address) {
    if (invalidationInfoOffset[address >> 1] == INVALIDATE_NONE)
      return false;
    invalidateSlowPath(address >> 1);
    return true;
  }

  bool invalidateByte(uint32_t address) {
    if (invalidationInfoOffset[address >> 1] == INVALIDATE_NONE)
      return false;
    invalidateSlowPath(address >> 1);
    return true;
  }
private:
  // The opcode cache is bigger than the memory size. We place an ILLEGAL_PC
  // pseudo instruction just past the end of memory. This saves
  // us from having to check for illegal pc values when incrementing the pc from
  // the previous instruction. Addition pseudo instructions come after this and
  // are use for communicating illegal states.
  OPCODE_TYPE *opcode;
  Operands *operands;
  unsigned char *invalidationInfo;
  unsigned char *invalidationInfoOffset;
  executionFrequency_t *executionFrequency;
  uint32_t getRamSizeShorts() const { return 1 << (ramSizeLog2 - 1); }

public:
  const uint32_t ramSizeLog2;
  const uint32_t ram_base;
  const uint32_t ramBaseMultiple;
  uint32_t vector_base;

  uint32_t syscallAddress;
  uint32_t exceptionAddress;

  Core(uint32_t RamSize, uint32_t RamBase);
  ~Core();

  void clearOpcode(uint32_t pc);
  void setOpcode(uint32_t pc, OPCODE_TYPE opc, unsigned size);
  void setOpcode(uint32_t pc, OPCODE_TYPE opc, Operands &ops, unsigned size);

  const Operands &getOperands(uint32_t pc) const { return operands[pc]; }
  const OPCODE_TYPE *getOpcodeArray() const { return opcode; }

  bool setSyscallAddress(uint32_t value);
  bool setExceptionAddress(uint32_t value);

  void initCache(OPCODE_TYPE decode, OPCODE_TYPE illegalPC,
                 OPCODE_TYPE illegalPCThread, OPCODE_TYPE runJit);

  void runJIT(uint32_t jitPc);

  uint32_t getRamSize() const { return 1 << ramSizeLog2; }

  bool updateExecutionFrequencyFromStub(uint32_t shiftedAddress) {
    const executionFrequency_t threshold = 128;
    if (++executionFrequency[shiftedAddress] > threshold) {
      return true;
    }
    return false;
  }

  void updateExecutionFrequency(uint32_t shiftedAddress) {
    if (updateExecutionFrequencyFromStub(shiftedAddress))
      runJIT(shiftedAddress);
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
    return (address >> ramSizeLog2) == ramBaseMultiple;
  }

  bool isValidPc(uint32_t address) const {
    return address < getRamSizeShorts();
  }

  uint32_t toPc(uint32_t address) const {
    return (address - ram_base) >> 1;
  }

  uint32_t fromPc(uint32_t pc) const {
    return (pc << 1) + ram_base;
  }

private:
  uint8_t *mem() {
    return reinterpret_cast<uint8_t*>(memory);
  }

  const uint8_t *mem() const {
    return reinterpret_cast<uint8_t*>(memory);
  }

  uint8_t *memOffset() {
    return reinterpret_cast<uint8_t*>(memoryOffset);
  }

  const uint8_t *memOffset() const {
    return reinterpret_cast<uint8_t*>(memoryOffset);
  }

public:
  uint32_t loadWord(uint32_t address) const
  {
    if (HOST_LITTLE_ENDIAN) {
      return *reinterpret_cast<const uint32_t*>((memOffset() + address));
    } else {
      return
        bswap32(*reinterpret_cast<const uint32_t*>((memOffset() + address)));
    }
  }

  int16_t loadShort(uint32_t address) const
  {
    return loadByte(address) | loadByte(address + 1) << 8;
  }

  uint8_t loadByte(uint32_t address) const
  {
    return memOffset()[address];
  }

  uint8_t &byte(uint32_t address)
  {
    return memOffset()[address];
  }

  bool storeWord(uint32_t value, uint32_t address)
  {
    if (HOST_LITTLE_ENDIAN) {
      *reinterpret_cast<uint32_t*>((memOffset() + address)) = value;
    } else {
      *reinterpret_cast<uint32_t*>((memOffset() + address)) = bswap32(value);
    }
    return invalidateWord(address);
  }

  bool storeShort(int16_t value, uint32_t address)
  {
    memOffset()[address] = static_cast<uint8_t>(value);
    memOffset()[address + 1] = static_cast<uint8_t>(value >> 8);
    return invalidateShort(address);
  }

  bool storeByte(uint8_t value, uint32_t address)
  {
    memOffset()[address] = value;
    return invalidateByte(address);
  }

  void writeMemory(uint32_t address, void *src, size_t size);

  Resource *allocResource(Thread &current, ResourceType type);

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

  unsigned getRunJitAddr() const {
    return (getRamSizeShorts() - 1) + RUN_JIT_ADDR_OFFSET;
  }

  unsigned getIllegalPCThreadAddr() const {
    return (getRamSizeShorts() - 1) + ILLEGAL_PC_THREAD_ADDR_OFFSET;
  }

  void finalize();
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
