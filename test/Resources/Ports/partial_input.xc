// RUN: xcc -O2 -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x80000 0x80100

#include <xs1.h>

buffered out port:32 p = XS1_PORT_8A;
buffered in port:32 q = XS1_PORT_8B;
clock c = XS1_CLKBLK_1;

int main() {
  unsigned time;
  int val;
  int tmp;
  configure_in_port(q, c);
  configure_out_port(p, c, 0);
  configure_clock_ref(c, 10);
  start_clock(c);
  p @ 10 <: 0x99542c28;
  q @ 10 :> val;
  val >>= 24;
  tmp = partin(q, 8);
  val |= tmp << 8;
  tmp = partin(q, 8);
  val |= tmp << 16;
  tmp = partin(q, 8);
  val |= tmp << 24;
  if (val != 0x99542c28)
    return 1;
  return 0;
}
