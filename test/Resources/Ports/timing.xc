// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe

#include <xs1.h>

in port p = XS1_PORT_1A;
clock c = XS1_CLKBLK_1;

#define ITERATIONS 100
#define EXPECTED ((ITERATIONS * 100 * 8) / 100)
#define TOLERANCE 5

int main() {
  timer t;
  unsigned t1, t2;
  unsigned diff;
  configure_clock_rate(c, 100, 8); //12.5MHz
  configure_in_port(p, c);
  start_clock(c);
  p :> void;
  t :> t1;
  for (unsigned i = 0; i < ITERATIONS; i++) {
  	p :> void;
  }
  t :> t2;
  diff = t2 - t1;
  if (diff < EXPECTED - TOLERANCE ||
  	  diff > EXPECTED + TOLERANCE)
  	return 1;
  return 0;
}
