// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _BitManip_h_
#define _BitManip_h_

inline uint32_t countLeadingZeros(uint32_t x)
{
#ifdef __GNUC__
  return x == 0 ? 0 : __builtin_clz(x);
#else
  unsigned i = 0;
  for (i = 32; x; i--) {
    x >>= 1;
  }
  return i;
#endif
}

inline int16_t bswap16(int16_t value)
{
  return ((value & 0xff00) >> 8) |
         ((value & 0xff) << 8);
}

inline uint32_t bswap32(uint32_t x)
{
#if __GNUC__ > 3
  return __builtin_bswap32(x);
#else
  return (x >> 24)
    | ((x >> 8) & 0x0000ff00)
    | ((x << 8) & 0x00ff0000)
    | (x << 24);
#endif
}

inline uint32_t bitReverse(uint32_t x)
{
  x =  ((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1);
  x = ((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2);
  x = ((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4);
  return bswap32(x);
}

inline uint32_t makeMask(uint32_t x)
{
  if (x > 31) {
    return 0xFFFFFFFFU;
  }
  return (1 << x) - 1;
}

#endif // _BitManip_h_
