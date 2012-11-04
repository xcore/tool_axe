// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _XE_h_
#define _XE_h_

#include <stdint.h>
#include <fstream>
#include <vector>

namespace axe {

class XE;

class XESector {
public:
  enum XBSectorType {
    XE_SECTOR_BINARY = 1,
    XE_SECTOR_ELF = 2,
    XE_SECTOR_CONFIG = 3,
    XE_SECTOR_GOTO = 5,
    XE_SECTOR_CALL = 6,
    XE_SECTOR_XN = 8,
    XE_SECTOR_LAST = 0x5555,
  };
private:
  XE &parent;
protected:
  uint64_t offset;
private:
  uint16_t type;
  uint64_t length;
public:
  XESector(XE &xe, uint64_t off, uint16_t t, uint64_t len)
    : parent(xe), offset(off), type(t), length(len) {}
  uint16_t getType() const { return type; };
  uint64_t getLength() const { return length; };
  bool getData(char *buf) const;
  XE &getParent() const { return parent; };
};

class XEElfSector : public XESector {
private:
  uint16_t node;
  uint16_t core;
  uint64_t address;
public:
  XEElfSector(XE &xe, uint64_t off, uint64_t len);
  uint16_t getNode() const { return node; };
  uint16_t getCore() const { return core; };
  uint64_t getAddress() const { return address; };
  bool getElfData(char *buf) const;
  uint64_t getElfSize() const { return getLength() - 12; }
};

class XECallOrGotoSector : public XESector {
private:
  uint16_t node;
  uint16_t core;
  uint64_t address;
public:
  XECallOrGotoSector(XE &xe, uint64_t off, uint16_t t, uint64_t len);
  uint16_t getNode() const { return node; };
  uint16_t getCore() const { return core; };
  uint64_t getAddress() const { return address; };  
};

class XE {
  
private:
  std::string filename;
  uint16_t version;
  std::ifstream s;
  std::vector<const XESector *>sectors;
  bool error;
  
  uint8_t ReadU8();
  uint16_t ReadU16();
  uint32_t ReadU32();
  uint64_t ReadU64();
  bool ReadHeader();
  const XESector *getSector(XESector::XBSectorType type) const;
  
  friend class XESector;
  friend class XEElfSector;
  friend class XECallOrGotoSector;
public:
  XE(const std::string &fname);
  ~XE();
  const std::string &getFileName() const { return filename; }
  const std::vector<const XESector *> &getSectors() { return sectors; }
  const XESector *getConfigSector() const {
    return getSector(XESector::XE_SECTOR_CONFIG);
  }
  const XESector *getXNSector() const {
    return getSector(XESector::XE_SECTOR_XN);
  }
  bool operator!() const {
    return error;
  }

  void close() {
    s.close();
  }
};
  
} // End axe namespace

#endif //_XE_h_
