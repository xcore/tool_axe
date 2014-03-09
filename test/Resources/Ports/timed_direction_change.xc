// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback XS1_PORT_16A XS1_PORT_16B

#include <xs1.h>
#include <stdlib.h>

port p = XS1_PORT_16A;
port q = XS1_PORT_16B;
clock clk = XS1_CLKBLK_1;

int main()
{
  int x;
  configure_in_port(q, clk);
  configure_out_port(p, clk, 0x1234);
  configure_clock_ref(clk, 10);
  start_clock(clk);
  par {
    {
      start_clock(clk);
      p @ 100 :> void;
    }
    {
      int x, y;
      q @ 99 :> x;
      q :> y;
      if (x != 0x1234)
        _Exit(1);
      if (y != 0)
        _Exit(1);
    }
  }
  return 0;
}
