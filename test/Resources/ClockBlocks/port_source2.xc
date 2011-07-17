// RUN: xcc -O2 -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x10000 0x10200

#include <xs1.h>

out port clk_out = XS1_PORT_1A;
in port clk_in = XS1_PORT_1B;
in port p = XS1_PORT_1C;
clock c = XS1_CLKBLK_1;

#define NUM_RISING 10
#define DELAY 50
#define EXPECTED (DELAY * 2 * NUM_RISING)
#define TOLERANCE 20

int main() {
  timer t;
  unsigned t1, t2;
  unsigned diff;
  clk_out <: 1;
  configure_clock_src(c, clk_in); //12.5MHz
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
  if (diff < EXPECTED - TOLERANCE ||
      diff > EXPECTED + TOLERANCE)
    return 1;
  return 0;
}
