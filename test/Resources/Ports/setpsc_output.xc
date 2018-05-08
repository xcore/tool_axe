// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback XS1_PORT_1A XS1_PORT_1F --loopback XS1_PORT_1B XS1_PORT_1E --loopback XS1_PORT_1C XS1_PORT_1G --loopback XS1_PORT_1D XS1_PORT_1H
// RUN: xcc -O2 -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: axe %t1.xe --loopback XS1_PORT_1A XS1_PORT_1F --loopback XS1_PORT_1B XS1_PORT_1E --loopback XS1_PORT_1C XS1_PORT_1G --loopback XS1_PORT_1D XS1_PORT_1H

#include <xs1.h>

port p_readyin = XS1_PORT_1A;
port p_readyout = XS1_PORT_1B;
port p_clk_port = XS1_PORT_1C;
in buffered port:8 p = XS1_PORT_1D;
clock p_clk = XS1_CLKBLK_1;

port q_readyin = XS1_PORT_1E;
port q_readyout = XS1_PORT_1F;
port q_clk_port = XS1_PORT_1G;
out buffered port:8 q = XS1_PORT_1H;
clock q_clk = XS1_CLKBLK_2;

clock clk_gen = XS1_CLKBLK_3;

#define MAGIC_VALUE 0x2e

// Defeat optimizers.
int identity(int x)
{
  asm("mov %0, %1" : "=r"(x) : "r"(x));
  return x;
}

int main()
{
  int received = 0;
  /* Configure generated clock */
  configure_clock_ref(clk_gen, 20);

  /* Configure p_clk */
  configure_port_clock_output(p_clk_port, clk_gen);

  /* Configure p */
  configure_clock_src(p_clk, p_clk_port);
  configure_in_port_handshake(p, p_readyin, p_readyout, p_clk);

  /* Configure q */
  configure_clock_src(q_clk, q_clk_port);
  configure_out_port_handshake(q, q_readyin, q_readyout, q_clk, 0);

  /* Start */
  start_clock(p_clk);
  start_clock(q_clk);
  start_clock(clk_gen);

  /* Transfer data */
  partout(q, identity(3), MAGIC_VALUE);
  partout(q, identity(5), MAGIC_VALUE >> 3);

  p :> received;
  if (received != MAGIC_VALUE)
    return 1;

  return 0;
}
