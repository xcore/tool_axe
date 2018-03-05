// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe

#include <xs1.h>
#include <stdio.h>

out port clk_out = XS1_PORT_1A;
// out port clk_out = 0x10200;
in port p = XS1_PORT_1B;
// in port p = 0x10000;
clock c = XS1_CLKBLK_1;
// __clock_t c = 0x106;

#define NUM_RISING 10
#define DELAY 50
#define EXPECTED (DELAY * 2 * NUM_RISING)
#define TOLERANCE 20

int main() {
  timer t;
  unsigned t1, t2;
  unsigned diff;
  clk_out <: 1;
  configure_clock_src(c, clk_out);
  configure_in_port(p, c);
  start_clock(c);
  t :> t1;
  par {
    {
      unsigned time = t1;
      unsigned value = 0;
      timer t;
      for (unsigned i = 0; i < (NUM_RISING * 2); i++) {
        t when timerafter(time+= DELAY) :> void;
        clk_out <: value;
        value = !value;
      }
    }
    {
      for (unsigned i = 0; i < NUM_RISING; i++) {
        p :> void;
      }
      t :> t2;
    }
  }

  diff = t2 - t1;
  printf("DIFF: %u\n", diff);
  printf("EXPECTED: %u\n", EXPECTED);
  if (diff < EXPECTED - TOLERANCE ||
      diff > EXPECTED + TOLERANCE)
    return 1;
  return 0;
}
