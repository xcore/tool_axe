// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x10000 0x10200
// RUN: xcc -O2 -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x10000 0x10200

#include <xs1.h>

port loopback = XS1_PORT_1B;
port readyIn = XS1_PORT_1A;
in buffered port:32 p = XS1_PORT_1C;
clock clk = XS1_CLKBLK_1;

int main()
{
  timer t;
  unsigned x;
  unsigned time;
  configure_clock_rate(clk, 100, 6);
  configure_in_port_strobed_slave(p, readyIn, clk);
  configure_out_port(loopback, clk, 0);
  start_clock(clk);
  t :> time;
  select {
  case t when timerafter(time + 200) :> void:
    break;
  case p :> void:
    return 1;
  }
  loopback <: 1;
  p :> void;
  return 0;
}
