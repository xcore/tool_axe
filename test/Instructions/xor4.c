/*
 * RUN: xcc -O2 -target=XCORE-200-EXPLORER %s -o %t1.xe
 * RUN: %sim %t1.xe
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define VERIFY(x) do { if(!(x)) { _Exit(1); }} while(0)

static inline uint32_t xor4(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
  uint32_t result;
  asm("xor4 %0, %1, %2, %3, %4" :
      "=r"(result) :
      "r"(a), "r"(b), "r"(c), "r"(d));
  return result;
}

int main() {
  VERIFY(xor4(2, 4, 8, 16) == 30);
  VERIFY(xor4(1, 4, 16, 64) == 85);
  VERIFY(xor4(2, 4, 2, 4) == 0);
  VERIFY(xor4(0xffffffff, 0xffff0000, 0xff00, 0) == 255);
  return 0;
}
