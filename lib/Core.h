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
#include <set>

namespace axe {

class Lock;
class Synchroniser;
class Chanend;
class ChanEndpoint;
class ClockBlock;
class Port;
class Timer;
class Tracer;

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

class Node;

enum ProcessorState {
  PS_RAM_BASE = 0x00b,
  PS_VECTOR_BASE = 0x10b,
  PS_BOOT_CONFIG = 0x30b,
  PS_BOOT_STATUS = 0x40b
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
  const uint32_t ramSizeLog2;
  uint32_t ramBaseMultiple;
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
  uint32_t bootConfig;
  uint32_t bootStatus;
  const uint8_t *rom;
  uint32_t romBase;
  uint32_t romSize;

  bool hasMatchingNodeID(ResourceID ID);
  void invalidateWordSlowPath(uint32_t address);
  void invalidateSlowPath(uint32_t shiftedAddress);
  void resetCaches();
  uint32_t getRamSizeShorts() const { return 1 << (ramSizeLog2 - 1); }

  std::set<uint32_t> breakpoints;
public:
  uint32_t vector_base;

  Core(uint32_t RamSize, uint32_t RamBase, bool tracing);
  ~Core();

  void setRamBaseMultiple(unsigned multiple);

  void clearOpcode(uint32_t pc);
  void setOpcode(uint32_t pc, OPCODE_TYPE opc, unsigned size);
  void setOpcode(uint32_t pc, OPCODE_TYPE opc, Operands &ops, unsigned size);

  const DecodeCache::State &getRamDecodeCache() const {
    return ramDecodeCache.getState();
  }

  const DecodeCache::State *getDecodeCacheContaining(uint32_t address) const;

  bool setBreakpoint(uint32_t value);
  void unsetBreakpoint(uint32_t value);
  bool isBreakpointAddress(uint32_t value) { return breakpoints.count(value); }

  // TODO should take address in order to handle ROM.
  void runJIT(uint32_t jitPc);

  uint32_t getRamSize() const { return 1 << ramSizeLog2; }
  uint32_t getRamSizeLog2() const { return ramSizeLog2; }
  uint32_t getRamBase() const { return getRamSize() * ramBaseMultiple; }
  uint32_t getRamBaseMultiple() const { return ramBaseMultiple; }
  uint32_t getRomSize() const { return romSize; }
  uint32_t getRomBase() const { return romBase; }
  
  bool isValidRamAddress(uint32_t address) const {
    return (address >> ramSizeLog2) == ramBaseMultiple;
  }
  
  bool isValidRomAddress(uint32_t address) const {
    return (address - romBase) < romSize;
  }
  
  bool isValidAddress(uint32_t address) const {
    return isValidRamAddress(address) || isValidRomAddress(address);
  }

  bool isValidRamPc(uint32_t address) const {
    return getRamDecodeCache().isValidPc(address);
  }

  uint32_t fromRamPc(uint32_t pc) const {
    return getRamDecodeCache().fromPc(pc);
  }
  
  uint32_t toRamPc(uint32_t address) const {
    return getRamDecodeCache().toPc(address);
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
  
  bool readMemory(uint32_t address, void *dst, size_t size);
  bool writeMemory(uint32_t address, const void *src, size_t size);

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

  bool setProcessorState(uint32_t reg, uint32_t value);
  bool getProcessorState(uint32_t reg, uint32_t &value);

  void finalize();
  void updateIDs();

  void setBootConfig(uint32_t value) { bootConfig = value; }

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
  Tracer &getTracer();
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
  
} // End axe namespace

#endif // _Core_h_
