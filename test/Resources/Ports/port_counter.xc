// RUN: xcc -O2 -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe

#include <xs1.h>

out port clk_out = XS1_PORT_1A;
in port p = XS1_PORT_1B;
clock c = XS1_CLKBLK_1;

static unsigned getts(void port p)
{
  unsigned value;
  asm("getts %0, res[%1]" : "=r"(value) : "r"(p));
  return value;
}

int main() {
  clk_out <: 0;
  configure_clock_src(c, clk_out);
  configure_in_port(p, c);
  start_clock(c);
  clk_out <: 1;
  sync(clk_out);
  p :> void;
  if (getts(p) != 0) {
    return 3;
  }
  clk_out <: 0;
  sync(clk_out);
  if (getts(p) != 0) {
    return 4;
  }
  clk_out <: 1;
  sync(clk_out);
  p :> void;
  if (getts(p) != 1) {
    return 5;
  }
  clk_out <: 0;
  sync(clk_out);
  if (getts(p) != 1) {
    return 6;
  }
  return 0;
}
