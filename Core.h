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
#include "DecodeCache.h"
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
    INTERPRET_ONE_ADDR_OFFSET = 3,
    ILLEGAL_PC_THREAD_ADDR_OFFSET = 4
  };
private:
  uint32_t * memoryOffset;
  unsigned char *invalidationInfoOffset;
  DecodeCache ramDecodeCache;
public:
  const uint32_t ramSizeLog2;
  const uint32_t ramBaseMultiple;
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
  const uint8_t *rom;
public:
  uint32_t romBase;
  uint32_t romSize;

private:
  bool hasMatchingNodeID(ResourceID ID);
  void invalidateWordSlowPath(uint32_t address);
  void invalidateSlowPath(uint32_t shiftedAddress);
  uint32_t getRamSizeShorts() const { return 1 << (ramSizeLog2 - 1); }

public:
  uint32_t vector_base;

  uint32_t syscallAddress;
  uint32_t exceptionAddress;

  Core(uint32_t RamSize, uint32_t RamBase);
  ~Core();

  void clearOpcode(uint32_t pc);
  void setOpcode(uint32_t pc, OPCODE_TYPE opc, unsigned size);
  void setOpcode(uint32_t pc, OPCODE_TYPE opc, Operands &ops, unsigned size);

  const DecodeCache::State &getRamDecodeCache() const {
    return ramDecodeCache.getState();
  }

  bool setSyscallAddress(uint32_t value);
  bool setExceptionAddress(uint32_t value);

  void resetCaches();

  void runJIT(uint32_t jitPc);

  uint32_t getRamSize() const { return 1 << ramSizeLog2; }
  uint32_t getRamBase() const { return getRamSize() * ramBaseMultiple; }

  bool updateExecutionFrequencyFromStub(uint32_t shiftedAddress) {
    const DecodeCache::executionFrequency_t threshold = 128;
    DecodeCache::executionFrequency_t *executionFrequency =
      ramDecodeCache.getState().executionFrequency;
    if (++executionFrequency[shiftedAddress] > threshold) {
      return true;
    }
    return false;
  }

  void updateExecutionFrequency(uint32_t shiftedAddress) {
    if (updateExecutionFrequencyFromStub(shiftedAddress))
      runJIT(shiftedAddress);
  }
  
  bool isValidRamAddress(uint32_t address) const {
    return (address >> ramSizeLog2) == ramBaseMultiple;
  }
  
  bool isValidRomAddress(uint32_t address) const {
    return (address - romBase) < romSize;
  }
  
  bool isValidAddress(uint32_t address) const {
    return isValidRamAddress(address) || isValidRomAddress(address);
  }

  bool isValidPc(uint32_t address) const {
    return address < getRamSizeShorts();
  }

  uint32_t fromPc(uint32_t pc) const {
    return (pc << 1) + getRamBase();
  }
  
  uint32_t toPc(uint32_t address) const {
    return (address - getRamBase()) >> 1;
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
  uint32_t loadRomWord(uint32_t address) const {
    uint32_t word;
    std::memcpy(&word, &rom[address - romBase], 4);
    if (HOST_LITTLE_ENDIAN) {
      return word;
    } else {
      return bswap32(word);
    }
  }

  uint16_t loadRomShort(uint32_t address) const {
    if (HOST_LITTLE_ENDIAN) {
      uint16_t halfWord;
      std::memcpy(&halfWord, &rom[address - romBase], 2);
      return halfWord;
    } else {
      return loadRomByte(address) | loadRomByte(address + 1) << 8;
    }
  }

  uint8_t loadRomByte(uint32_t address) const {
    return rom[address - romBase];
  }

  uint32_t loadRamWord(uint32_t address) const {
    if (HOST_LITTLE_ENDIAN) {
      return *reinterpret_cast<const uint32_t*>((memOffset() + address));
    } else {
      return
        bswap32(*reinterpret_cast<const uint32_t*>((memOffset() + address)));
    }
  }

  uint16_t loadRamShort(uint32_t address) const {
    return loadRamByte(address) | loadRamByte(address + 1) << 8;
  }

  uint8_t loadRamByte(uint32_t address) const {
    return memOffset()[address];
  }
  
  uint8_t loadByte(uint32_t address) const {
    if (isValidRamAddress(address))
      return loadRamByte(address);
    return loadRomByte(address);
  }
  
  uint16_t loadShort(uint32_t address) const {
    if (isValidRamAddress(address))
      return loadRamShort(address);
    return loadRomShort(address);
  }

  bool invalidateWordCheck(uint32_t address) {
    uint16_t info;
    std::memcpy(&info, &invalidationInfoOffset[address >> 1], sizeof(info));
    if (info == (DecodeCache::INVALIDATE_NONE | (DecodeCache::INVALIDATE_NONE << 8)))
      return false;
    return true;
  }

  bool invalidateWord(uint32_t address) {
    if (!invalidateWordCheck(address))
      return false;
    invalidateWordSlowPath(address >> 1);
    return true;
  }

  bool invalidateShortCheck(uint32_t address) {
    if (invalidationInfoOffset[address >> 1] == DecodeCache::INVALIDATE_NONE)
      return false;
    return true;
  }

  bool invalidateShort(uint32_t address) {
    if (!invalidateShortCheck(address))
      return false;
    invalidateSlowPath(address >> 1);
    return true;
  }

  bool invalidateByteCheck(uint32_t address) {
    if (invalidationInfoOffset[address >> 1] == DecodeCache::INVALIDATE_NONE)
      return false;
    return true;
  }

  bool invalidateByte(uint32_t address) {
    if (!invalidateByteCheck(address))
      return false;
    invalidateSlowPath(address >> 1);
    return true;
  }

  uint8_t *ramBytePtr(uint32_t address) {
    return &memOffset()[address];
  }
  
  const uint8_t *romBytePtr(uint32_t address) {
    return &rom[address - romBase];
  }
  
  const uint8_t *memPtr(uint32_t address) {
    return isValidRamAddress(address) ? ramBytePtr(address) :
                                        romBytePtr(address);
  }

  void storeWord(uint32_t value, uint32_t address)
  {
    if (HOST_LITTLE_ENDIAN) {
      *reinterpret_cast<uint32_t*>((memOffset() + address)) = value;
    } else {
      *reinterpret_cast<uint32_t*>((memOffset() + address)) = bswap32(value);
    }
  }

  void storeShort(int16_t value, uint32_t address)
  {
    memOffset()[address] = static_cast<uint8_t>(value);
    memOffset()[address + 1] = static_cast<uint8_t>(value >> 8);
  }

  void storeByte(uint8_t value, uint32_t address)
  {
    memOffset()[address] = value;
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

  unsigned getInterpretOneAddr() const {
    return (getRamSizeShorts() - 1) + INTERPRET_ONE_ADDR_OFFSET;
  }

  unsigned getIllegalPCThreadAddr() const {
    return (getRamSizeShorts() - 1) + ILLEGAL_PC_THREAD_ADDR_OFFSET;
  }

  void finalize();
  void updateIDs();

  void setRom(const uint8_t *data, uint32_t romBase, uint32_t romSize);
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
