// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe

#include <xs1.h>

port p = XS1_PORT_1A;
clock c = XS1_CLKBLK_1;

int main()
{
  timer t;
  int time;
  t :> time;
  configure_in_port(p, c);
  par {
    p :> void;
    {
      t when timerafter(time += 100) :> void;
      start_clock(c);
    }
  }
  return 0;
}
