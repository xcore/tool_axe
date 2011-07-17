// RUN: xcc -O2 -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x10000 0x10200

#include <xs1.h>

port p = XS1_PORT_1A;
port q = XS1_PORT_1B;
clock c = XS1_CLKBLK_1;

int main() {
  unsigned time;
  int x;
  configure_in_port(p, c);
  configure_out_port(q, c, 0);
  configure_clock_ref(c, 10);
  start_clock(c);
  q @ 10 <: 1;
  p @ 9 :> x;
  if (x != 0)
    return 1;
  p @ 10 :> x;
  if (x != 1)
    return 2;
  return 0;
}
