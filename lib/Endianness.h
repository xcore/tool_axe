// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Endianness_h_
#define _Endianness_h_

#include <stdint.h>
#include <cstdlib>
#include <boost/detail/endian.hpp>
#include "BitManip.h"

#if defined(BOOST_LITTLE_ENDIAN)
#define HOST_LITTLE_ENDIAN 1
#elif defined(BOOST_BIG_ENDIAN)
#define HOST_LITTLE_ENDIAN 0
#else
#error "Unknown endianness"
#endif

namespace axe {

namespace endianness {
  inline uint32_t read32le(const uint8_t *p) {
    uint32_t val;
    std::memcpy(&val, p, sizeof(val));
    if (!HOST_LITTLE_ENDIAN)
      val = bswap32(val);
    return val;
  }

  inline uint16_t read16le(const uint8_t *p) {
    if (HOST_LITTLE_ENDIAN) {
      uint16_t val;
      std::memcpy(&val, p, sizeof(val));
      return val;
    }
    return p[0] | (p[1] << 8);
  }

  inline void write32le(uint8_t *p, uint32_t val) {
    if (!HOST_LITTLE_ENDIAN)
      val = bswap32(val);
    std::memcpy(p, &val, sizeof(val));
  }

  inline void write16le(uint8_t *p, uint16_t val) {
    if (HOST_LITTLE_ENDIAN) {
      std::memcpy(p, &val, sizeof(val));
      return;
    }
    p[0] = val;
    p[1] = val >> 8;
  }
}

} // End axe namespace

#endif // _Endianness_h_
