// RUN: xcc -O2 -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x10000 0x10200

#include <xs1.h>
#include <stdlib.h>

in buffered port:32 p = XS1_PORT_1C;
port readyOut = XS1_PORT_1A;
in buffered port:32 loopback = XS1_PORT_1B;
clock clk = XS1_CLKBLK_1;

int main()
{
  unsigned x;
  configure_clock_rate(clk, 100, 6);
  configure_in_port_strobed_master(p, readyOut, clk);
  configure_in_port(loopback, clk);
  start_clock(clk);
  loopback :> x;
  p @ 5 + 32 :> void;
  loopback :> x;
  if (x != 0xffffffe1)
    _Exit(1);
  loopback :> x;
  if (x != 0xffffffff)
    _Exit(2);
  loopback :> x;
  if (x != 0xffffffff)
    _Exit(3);
  loopback :> x;
  if (x != 0x0000001f)
    _Exit(4);
  return 0;
}
