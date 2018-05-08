// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: %sim %t1.xe

#include <xs1.h>

out port clk_out = XS1_PORT_1A;
in port p = XS1_PORT_1B;
clock c = XS1_CLKBLK_1;
clock c2 = XS1_CLKBLK_2;

int main() {
  timer t;
  unsigned time;
  unsigned diff;
  set_clock_div(c2, 10);
  configure_out_port(clk_out, c2, 1);
  configure_clock_src(c, clk_out);
  configure_in_port(p, c);
  start_clock(c2);
  start_clock(c);
  t :> time;
  clk_out <: 0;
  // This should timeout
  select {
  case p :> void:
    return 1;
  case t when timerafter(time+100) :> void:
    break;
  }
  t :> time;
  clk_out <: 1;
  // This shouldn't timeout
  select {
  case p :> void:
    break;
  case t when timerafter(time+100) :> void:
    return 2;
  }
  return 0;
}
