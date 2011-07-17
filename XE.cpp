// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "XE.h"
#include <cstring>
#include <iostream>

bool XESector::getData(char *buf) const
{
  getParent().s.seekg(offset);
  getParent().s.read(buf, length);
  return getParent().s.good();
}

XEElfSector::XEElfSector(XE &xe, uint64_t off, uint64_t len)
  : XESector(xe, off, XE_SECTOR_ELF, len)
{
  node = xe.ReadU16();
  core = xe.ReadU16();
  address = xe.ReadU64();
}

bool XEElfSector::getElfData(char *buf) const
{
  getParent().s.seekg(offset + 12);
  getParent().s.read(buf, getElfSize());
  return getParent().s.good();
}

XE::XE(const char *filename)
  : s(filename, std::ifstream::in | std::ifstream::binary),
    error(false)
{
  if (!s) {
    error = true;
    return;
  }
  char magic[4];
  s.read(magic, 4);
  if (std::memcmp(magic, "XMOS", 4) != 0) {
    error = true;
    return;
  }
  version = ReadU16();
  // Skip padding
  s.seekg(2, std::ios_base::cur);
  // Read sector headers
  while (!ReadHeader()) {}
}

XE::~XE()
{
  for (std::vector<const XESector *>::iterator it = sectors.begin(),
                                               end = sectors.end();
      it != end; ++it) {
    delete (*it);
  }
}

const XESector *XE::getConfigSector() const
{
  for (std::vector<const XESector *>::const_iterator it = sectors.begin(),
       end = sectors.end(); it != end; ++it) {
    switch((*it)->getType()) {
    case XESector::XE_SECTOR_CONFIG:
      return *it;
    }
  }
  return 0;
}


uint8_t XE::ReadU8()
{
  uint8_t value = s.get();
  return value;
}

uint16_t XE::ReadU16()
{
  char data[2];
  s.read(data, 2);
  return data[0] | (data[1] << 8);
}

uint32_t XE::ReadU32()
{
  char data[4];
  s.read(data, 4);
  return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

uint64_t XE::ReadU64()
{
  char data[8];
  s.read(data, 8);
  return (uint8_t)data[0] | ((uint8_t)data[1] << 8) | ((uint8_t)data[2] << 16) | ((uint8_t)data[3] << 24) |
  ((uint64_t)(data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24)) << 32);
}

bool XE::ReadHeader() {
  uint16_t type = ReadU16();
  // Skip
  s.seekg(2, std::ios_base::cur);
  uint64_t length = ReadU64();
  uint8_t padding = 0;
  if (length > 0) {
    padding = ReadU8();    
    // Skip
    s.seekg(3, std::ios_base::cur);
  }
  if (!s) {
    error = true;
    return true;
  }
  uint64_t off = s.tellg();
  switch (type) {
  case XESector::XE_SECTOR_ELF:
    sectors.push_back(new XEElfSector(*this, off, length - padding));
    break;
  case XESector::XE_SECTOR_LAST:
    return true;
  default:
    sectors.push_back(new XESector(*this, off, type, length - padding));
  }
  // Skip section
  s.seekg(off + length - 4);
  return false;
}
