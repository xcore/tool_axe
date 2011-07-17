// RUN: xcc -O2 -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe

#include <xs1.h>

out port p = XS1_PORT_1A;
clock c = XS1_CLKBLK_1;

#define EXPECTED ((100 * 100 * 8) / 100)
#define TOLERANCE 5

int main() {
  timer t;
  unsigned t1, t2;
  unsigned diff;
  configure_clock_rate(c, 100, 8); //12.5MHz
  configure_out_port(p, c, 0);
  start_clock(c);
  p <: 0;
  p <: 0;
  t :> t1;
  for (unsigned i = 0; i < 100; i++) {
  	p <: 0;
  }
  t :> t2;
  diff = t2 - t1;
  if (diff < EXPECTED - TOLERANCE ||
  	  diff > EXPECTED + TOLERANCE)
  	return 1;
  return 0;
}
