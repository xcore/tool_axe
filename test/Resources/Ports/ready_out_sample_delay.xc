// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback PORT_8A PORT_8B --loopback PORT_1A PORT_1E --loopback PORT_1B PORT_1D
// RUN: xcc -O2 -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: axe %t1.xe --loopback PORT_8A PORT_8B --loopback PORT_1A PORT_1E --loopback PORT_1B PORT_1D

#include <xs1.h>
#include <stdlib.h>

#define VERIFY(x) do { if (!(x)) /*_Exit(1)*/; } while(0)

// The following ports should be connected in loopback:
// PORT_8A <-> PORT_8B
// PORT_1A <-> PORT_1E
// PORT_1B <-> PORT_1D

in buffered port:8 p = XS1_PORT_8A;
port readyInLoop = XS1_PORT_1A;
port readyOutLoop = XS1_PORT_1B;
clock slowClk = XS1_CLKBLK_1;

out buffered port:32 q = XS1_PORT_8B;
port q_readyIn = XS1_PORT_1E;
port q_readyOut = XS1_PORT_1D;
port q_clksrc = XS1_PORT_1F;
clock q_clk = XS1_CLKBLK_2;

int main()
{
  int tmp1, tmp2;
  configure_in_port(readyOutLoop, slowClk);
  configure_out_port(readyInLoop, slowClk, 0);
  configure_out_port(q_clksrc, slowClk, 1);
  configure_clock_rate(slowClk, 100, 16);

  configure_clock_src(q_clk, q_clksrc);
  set_port_sample_delay(q);
  configure_out_port_handshake(q, q_readyIn, q_readyOut, q_clk, 0);
  q_clksrc <: 1;
  readyInLoop <: 0;
  q <: 0x52a8c1f5;
  start_clock(slowClk);
  start_clock(q_clk);
  readyInLoop <: 0;
  q_clksrc <: 0;

  readyInLoop <: 1;
  q_clksrc <: 1;
  sync(q_clksrc);
  readyOutLoop :> tmp1;
  p :> tmp2;
  VERIFY(tmp1 == 1);
  VERIFY(tmp2 == 0x52);

  readyInLoop <: 1;
  q_clksrc <: 0;

  readyInLoop <: 0;
  q_clksrc <: 1;
  sync(q_clksrc);
  readyOutLoop :> tmp1;
  p :> tmp2;
  VERIFY(tmp1 == 1);
  VERIFY(tmp2 == 0x52);

  q_clksrc <: 0;
  q_clksrc <: 1;
  sync(q_clksrc);
  readyOutLoop :> tmp1;
  p :> tmp2;
  VERIFY(tmp1 == 1);
  VERIFY(tmp2 == 0xa8);

  q_clksrc <: 0;
  q_clksrc <: 1;
  sync(q_clksrc);
  readyOutLoop :> tmp1;
  p :> tmp2;
  VERIFY(tmp1 == 1);
  VERIFY(tmp2 == 0xa8);

  return 0;
}
