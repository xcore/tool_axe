/*
 * RUN: xcc -O1 -target=XCORE-200-EXPLORER %s -o %t1.xe
 * RUN: %sim %t1.xe
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define VERIFY(x) do { if(!(x)) { _Exit(1); }} while(0)
#define STRINGIFY(x) #x


void print_bits(uint64_t num) {
  for (int i=0; i<64; i++) {
    int x = (num >> (63 - i)) & 1;
    if (i == 32) {
      printf(" | ");
    }
    printf("%d", x);
  }
  printf("\n");
}


#define ZIP_CASE(D, E, S) case S: asm("zip %0, %1, "STRINGIFY(S) : "=r"(D), "=r"(E) ); break
uint64_t zip(uint64_t de, int s)
{
  uint32_t e = de;
  uint32_t d = de >> 32;

  switch (s) {
    ZIP_CASE(d, e, 0);
    ZIP_CASE(d, e, 1);
    ZIP_CASE(d, e, 2);
    ZIP_CASE(d, e, 3);
    ZIP_CASE(d, e, 4);
    ZIP_CASE(d, e, 5);
    ZIP_CASE(d, e, 6);
    ZIP_CASE(d, e, 7);
    ZIP_CASE(d, e, 8);
    ZIP_CASE(d, e, 9);
    ZIP_CASE(d, e, 10);
    ZIP_CASE(d, e, 11);
  }

  uint64_t result = d;
  result = result << 32;
  result = result | e;
  return result;
}


#define UNZIP_CASE(D, E, S) case S: asm("unzip %0, %1, "STRINGIFY(S) : "=r"(D), "=r"(E) ); break
uint64_t unzip(uint64_t de, int s)
{
  uint32_t e = de;
  uint32_t d = de >> 32;

  switch (s) {
    UNZIP_CASE(d, e, 0);
    UNZIP_CASE(d, e, 1);
    UNZIP_CASE(d, e, 2);
    UNZIP_CASE(d, e, 3);
    UNZIP_CASE(d, e, 4);
    UNZIP_CASE(d, e, 5);
    UNZIP_CASE(d, e, 6);
    UNZIP_CASE(d, e, 7);
    UNZIP_CASE(d, e, 8);
    UNZIP_CASE(d, e, 9);
    UNZIP_CASE(d, e, 10);
    UNZIP_CASE(d, e, 11);
  }

  uint64_t result = d;
  result = result << 32;
  result = result | e;
  return result;
}


int main()
{
  VERIFY(zip(0xffffffffffffffff, 0) == 0xffffffffffffffff);
  VERIFY(zip(0xffffffff00000000, 0) == 0xaaaaaaaaaaaaaaaa);
  VERIFY(zip(0xffffffff00000000, 1) == 0xcccccccccccccccc);
  VERIFY(zip(0xffffffff00000000, 2) == 0xf0f0f0f0f0f0f0f0);
  VERIFY(zip(0xffffffff00000000, 3) == 0xff00ff00ff00ff00);
  VERIFY(zip(0xffffffff00000000, 4) == 0xffff0000ffff0000);

  for (int i=5; i<=11; i++) {
    VERIFY(zip(0xffffffff00000000, i) == 0xffffffff00000000);
  }
  VERIFY(zip(0x0123456789abcdef, 0) == 0x40434c4f70737c7f);
  VERIFY(zip(0x0123456789abcdef, 1) == 0x20252a2f70757a7f);
  VERIFY(zip(0x0123456789abcdef, 2) == 0x8192a3b4c5d6e7f);
  VERIFY(zip(0x0123456789abcdef, 3) == 0x18923ab45cd67ef);
  VERIFY(zip(0x0123456789abcdef, 4) == 0x12389ab4567cdef);
  for (int i=5; i<=11; i++) {
    VERIFY(zip(0x0123456789abcdef, i) == 0x0123456789abcdef);
  }

  VERIFY(unzip(0xffffffffffffffff, 0) == 0xffffffffffffffff);
  VERIFY(unzip(0xaaaaaaaaaaaaaaaa, 0) == 0xffffffff00000000);
  VERIFY(unzip(0xcccccccccccccccc, 1) == 0xffffffff00000000);
  VERIFY(unzip(0xf0f0f0f0f0f0f0f0, 2) == 0xffffffff00000000);
  VERIFY(unzip(0xff00ff00ff00ff00, 3) == 0xffffffff00000000);
  VERIFY(unzip(0xffff0000ffff0000, 4) == 0xffffffff00000000);

  for (int i=5; i<=11; i++) {
    VERIFY(unzip(0xffffffff00000000, i) == 0x0000000000000000);
  }
  VERIFY(unzip(0x40434c4f70737c7f, 0) == 0x0123456789abcdef);
  VERIFY(unzip(0x20252a2f70757a7f, 1) == 0x0123456789abcdef);
  VERIFY(unzip(0x8192a3b4c5d6e7f, 2) == 0x0123456789abcdef);
  VERIFY(unzip(0x18923ab45cd67ef, 3) == 0x0123456789abcdef);
  VERIFY(unzip(0x12389ab4567cdef, 4) == 0x0123456789abcdef);
  for (int i=5; i<=11; i++) {
    VERIFY(unzip(0x0123456789abcdef, i) == 0x0000000000000000);
  }

  return 0;
}
