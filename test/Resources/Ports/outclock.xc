// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x10200 0x10000
// RUN: xcc -O2 -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x10200 0x10000

#include <xs1.h>

out port p = XS1_PORT_1A;
in port q = XS1_PORT_1B;
clock c = XS1_CLKBLK_1;

int main() {
  configure_clock_ref(c, 10);
  configure_port_clock_output(p, c);
  start_clock(c);
  q when pinseq(0) :> void;
  q when pinseq(1) :> void;
  return 0;
}
