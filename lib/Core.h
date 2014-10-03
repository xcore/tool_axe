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
#include "Range.h"
#include "Endianness.h"
#include "WatchpointException.h"
#include "WatchpointManager.h"

namespace axe {

class Lock;
class Synchroniser;
class Chanend;
class ChanEndpoint;
class ClockBlock;
class Port;
class Timer;
class Tracer;

class ProcessorNode;

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
  uint8_t * memoryOffset;
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
  uint8_t * const memory;
  unsigned coreNumber;
  ProcessorNode *parent;
  std::string codeReference;
  unsigned bootModePins;
  uint32_t bootStatus;
  const uint8_t *rom;
  uint32_t romBase;
  uint32_t romSize;
  bool tracingEnabled;

  bool hasMatchingNodeID(ResourceID ID);
  void invalidateWordSlowPath(uint32_t address);
  void invalidateSlowPath(uint32_t shiftedAddress);
  void resetCaches();
  uint32_t getRamSizeShorts() const { return 1 << (ramSizeLog2 - 1); }

  std::set<uint32_t> breakpoints;
  WatchpointManager watchpoints;
public:
  uint32_t vector_base;

  Core(uint32_t RamSize, uint32_t RamBase, bool tracing);
  Core(const Core &) = delete;
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
  void clearBreakpoints() { breakpoints.clear(); }
  bool isBreakpointAddress(uint32_t value) const {
    return breakpoints.count(value);
  }

  bool setWatchpoint(WatchpointType type, uint32_t lowAddress, uint32_t highAddress);
  void unsetWatchpoint(WatchpointType type, uint32_t lowAddress, uint32_t highAddress);
  void clearWatchpoints() { watchpoints.clearWatchpoints(); };
  bool onWatchpoint(WatchpointType t, uint32_t address);

  bool jitEnabled;
  void disableJIT();
  void enableJIT();

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
    return endianness::read32le(&rom[address - romBase]);
  }

  uint16_t loadRomShort(uint32_t address) const {
    return endianness::read16le(&rom[address - romBase]);
  }

  uint8_t loadRomByte(uint32_t address) const {
    return rom[address - romBase];
  }

  uint32_t loadRamWord(uint32_t address) const {
    return endianness::read32le(memOffset() + address);
  }

  uint16_t loadRamShort(uint32_t address) const {
    return endianness::read16le(memOffset() + address);
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

  bool invalidateRange(uint32_t begin, uint32_t end);

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
    endianness::write32le(memOffset() + address, value);
  }

  void storeShort(int16_t value, uint32_t address)
  {
    endianness::write16le(memOffset() + address, value);
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

  void setBootModePins(unsigned value) { bootModePins = value; }

  void setRom(const uint8_t *data, uint32_t romBase, uint32_t romSize);

  /// Set the parent of the current core. updateIDs() must be called to update
  /// The core IDs and the channel end resource IDs.
  void setParent(ProcessorNode *n) {
    parent = n;
  }
  void setCoreNumber(unsigned value) { coreNumber = value; }
  unsigned getCoreNumber() const { return coreNumber; }
  uint32_t getCoreID() const;
  const ProcessorNode *getParent() const { return parent; }
  Tracer *getTracer();
  ProcessorNode *getParent() { return parent; }
  void dumpPaused() const;
  Thread &getThread(unsigned num) { return thread[num]; }
  const Thread &getThread(unsigned num) const { return thread[num]; }
  range<Thread *> getThreads() {
    return make_range(thread, thread + NUM_THREADS);
  }
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
  range<port_iterator> getPorts() {
    return make_range(port_begin(), port_end());
  }
};
  
} // End axe namespace

#endif // _Core_h_
