// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x80000 0x80100

#include <xs1.h>
#include <stdlib.h>

#define VERIFY(x) do { if (!(x)) _Exit(1); } while(0)

port p = XS1_PORT_8A;
port q = XS1_PORT_8B;

int main() {
  p <: 0xab;
  sync(p);
  VERIFY(peek(p) == 0xab);
  VERIFY(peek(q) == 0xab);
  return 0;
}
