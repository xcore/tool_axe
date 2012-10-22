// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe

#include <xs1.h>

port p = XS1_PORT_1A;
clock c = XS1_CLKBLK_1;

int main() {
  unsigned time;
  set_clock_div(c, 10);
  configure_in_port(p, c);
  start_clock(c);
  for (unsigned i = 0; i < 10; i++) {
    p :> void @ time;
    if (time != i)
      return 1;
  }
  return 0;
}
