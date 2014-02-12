// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback XS1_PORT_1A XS1_PORT_1B

#include <xs1.h>

// A timed conditional input on a buffer port waits until the time and then
// behaves the same as a untimed conditional input.

buffered out port:1 p = XS1_PORT_1A;
buffered in port:1 q = XS1_PORT_1B;
clock clk = XS1_CLKBLK_1;

int main()
{
  unsigned t;
  int x;
  configure_in_port(q, clk);
  configure_out_port(p, clk, 0);
  configure_clock_ref(clk, 10);
  par {
    {
      start_clock(clk);
      p @ 100 <: 1;
      p @ 200 <: 0;
      p @ 300 <: 1;
    }
    q @ 150 when pinseq(0) :> x @t;
  }
  if (x != 0)
    return 1;
  if (t != 200)
    return 2;
  return 0;
}
