// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x80000 0x80100 --loopback 0x10200 0x10600 --loopback 0x10000 0x10300 --loopback 0x10100 0x10400

#include <xs1.h>

// The following ports should be connected in loopback:
// PORT_8A <-> PORT_8B
// PORT_1A <-> PORT_1E
// PORT_1B <-> PORT_1D
// PORT_1C <-> PORT_1F

in buffered port:32 p = XS1_PORT_8A;
port p_readyIn = XS1_PORT_1A;
port p_readyOut = XS1_PORT_1B;
port p_outclk = XS1_PORT_1C;
clock p_clk = XS1_CLKBLK_1;

out buffered port:32 q = XS1_PORT_8B;
port q_readyIn = XS1_PORT_1D;
port q_readyOut = XS1_PORT_1E;
port q_clksrc = XS1_PORT_1F;
clock q_clk = XS1_CLKBLK_2;

int main()
{
  unsigned x;
  unsigned time;
  configure_clock_rate(p_clk, 100, 24);
  configure_in_port_handshake(p, p_readyIn, p_readyOut, p_clk);
  configure_port_clock_output(p_outclk, p_clk);

  configure_clock_src(q_clk, q_clksrc);
  configure_out_port_handshake(q, q_readyIn, q_readyOut, q_clk, 0);

  start_clock(q_clk);
  start_clock(p_clk);
  q <: 0x2CC94916;
  q <: 0xDE298055;
  q <: 0xF6525045;

  p :> x;
  if (x != 0x2CC94916)
    return 1;
  p :> x;
  if (x != 0xDE298055)
    return 2;
  p :> x;
  if (x != 0xF6525045)
    return 3;

  q @ 32 <: 0xCC2B4EF9;
  p :> x @ time;
  if (x != 0xCC2B4EF9)
    return 3;
  if (time != 35)
    return 4;
  return 0;
}
