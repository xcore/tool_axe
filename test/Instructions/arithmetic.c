/*
 * RUN: xcc -O2 -target=XC-5 %s -o %t1.xe
 * RUN: axe %t1.xe
 */
#include <stdint.h>
#include <stdlib.h>
#define VERIFY(x) do { if(!(x)) { _Exit(1); }} while(0)

static inline void crc32(uint32_t *checksum, uint32_t data, uint32_t poly)
{
  asm("crc32 %0, %2, %3" :
      "=r"(*checksum) :
      "0"(*checksum), "r"(data), "r"(poly));
}

static inline uint32_t crc8(uint32_t *checksum, uint32_t data, uint32_t poly)
{
  uint32_t shifted;
  asm("crc8 %0, %1, %3, %4" :
      "=r"(*checksum), "=r"(shifted) :
      "0"(*checksum), "r"(data), "r"(poly));
  return shifted;
}

static inline uint32_t shr(uint32_t a, uint32_t b)
{
  uint32_t result;
  asm("shr %0, %1, %2" :
      "=r"(result) :
      "r"(a), "r"(b));
  return result;
}

static inline uint32_t ashr(uint32_t a, uint32_t b)
{
  uint32_t result;
  asm("ashr %0, %1, %2" :
      "=r"(result) :
      "r"(a), "r"(b));
  return result;
}

static inline uint32_t shl(uint32_t a, uint32_t b)
{
  uint32_t result;
  asm("shl %0, %1, %2" :
      "=r"(result) :
      "r"(a), "r"(b));
  return result;
}

#define CRC32(a, b, c) crc32(&a, b, c)
#define CRC8(a, b, c) crc8(&a, b, c)

int main() {
  uint32_t x = 55568178;
  uint32_t data = 7880939;
  uint32_t poly = 9335255;
  CRC32(x, data, poly);
  VERIFY(x == 10352975);
  x = 55568178;
  data = 7880939;
  for (unsigned i = 0; i < 4; i++) {
    data = CRC8(x, data, poly);
  }
  VERIFY(x == 10352975);

  VERIFY(shr(6, 1) == 3);
  VERIFY(shr(0xffffffff, 31) == 1);
  VERIFY(shr(0xffffffff, 32) == 0);
  VERIFY(shr(0xffffffff, 0xffffffff) == 0);

  VERIFY(ashr(6, 1) == 3);
  VERIFY(ashr(0xffffffff, 31) == 0xffffffff);
  VERIFY(ashr(0x80000000, 32) == 0xffffffff);
  VERIFY(ashr(0x7fffffff, 32) == 0);
  VERIFY(ashr(0x7fffffff, 0xffffffff) == 0);
  VERIFY(ashr(0x80000000, 0xffffffff) == 0xffffffff);

  VERIFY(shl(6, 2) == 24);
  VERIFY(shl(0xffffffff, 31) == 0x80000000);
  VERIFY(shl(0xffffffff, 32) == 0);
  VERIFY(shl(0xffffffff, 0xffffffff) == 0);

  return 0;
}
