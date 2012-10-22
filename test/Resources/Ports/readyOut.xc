// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x10000 0x10200

#include <xs1.h>

out buffered port:32 p = XS1_PORT_1C;
port readyOut = XS1_PORT_1A;
in buffered port:32 loopback = XS1_PORT_1B;
clock clk = XS1_CLKBLK_1;

int main()
{
  unsigned x;
  configure_clock_rate(clk, 100, 6);
  configure_out_port_strobed_master(p, readyOut, clk, 0);
  configure_in_port(loopback, clk);
  p @ 5 <: 0;
  start_clock(clk);
  loopback :> x;
  if (x != 0xffffffe0)
    return 1;
  loopback :> x;
  if (x != 0x1f)
    return 2;
  return 0;
}
