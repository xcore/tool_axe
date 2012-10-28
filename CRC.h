// Copyright (c) 2011-12, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _CRC_h_
#define _CRC_h_

#include <climits>

namespace axe {

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

inline uint32_t crc32(uint32_t checksum, uint32_t data, uint32_t poly)
{
  return crc<uint32_t>(checksum, data, poly);
}

inline uint32_t crc8(uint32_t checksum, uint8_t data, uint32_t poly)
{
  return crc<uint8_t>(checksum, data, poly);
}
  
} // End axe namespace

#endif //_CRC_h_
