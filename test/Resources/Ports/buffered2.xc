// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x80000 0x80100

#include <xs1.h>

buffered out port:32 p = XS1_PORT_8A;
in port q = XS1_PORT_8B;
clock c = XS1_CLKBLK_1;

int main() {
  unsigned time;
  int val;
  configure_in_port(q, c);
  configure_out_port(p, c, 0);
  configure_clock_ref(c, 10);
  start_clock(c);
  p @ 10 <: 0x99542c28;
  q @10 :>>> val;
  q :>>> val;
  q :>>> val;
  q :>>> val;
  if (val != 0x99542c28)
    return 1;
  return 0;
}
