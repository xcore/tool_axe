// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x80000 0x80100
// RUN: xcc -O2 -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x80000 0x80100

#include <xs1.h>

buffered out port:32 p = XS1_PORT_8A;
buffered in port:32 q = XS1_PORT_8B;
clock c = XS1_CLKBLK_1;

int main() {
  unsigned tmp;
  configure_in_port(q, c);
  configure_out_port(p, c, 0);
  configure_clock_ref(c, 10);
  start_clock(c);
  p @ 10 <: 0x99542c28;
  set_port_shift_count(q, 24);
  q @ 10 :> tmp;
  if ((tmp >> 24) != 0x28)
    return 1;
  set_port_shift_count(q, 8);
  q @ 12 :> tmp;
  if ((tmp >> 24) != 0x54)
    return 2;
  return 0;
}
