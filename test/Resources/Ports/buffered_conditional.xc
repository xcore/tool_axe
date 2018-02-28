// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback XS1_PORT_8A XS1_PORT_8B
// RUN: xcc -O2 -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: axe %t1.xe --loopback XS1_PORT_8A XS1_PORT_8B

#include <xs1.h>

out port p = XS1_PORT_8A;
buffered in port:32 q = XS1_PORT_8B;
clock c = XS1_CLKBLK_1;

int main() {
  unsigned time;
  int tmp, t;
  configure_in_port(q, c);
  configure_out_port(p, c, 0);
  configure_clock_ref(c, 10);
  p <: 0x28;
  par {
    {
      start_clock(c);
      p <: 0x2c;
      p <: 0x54;
      p <: 0x99;
      p <: 0x12;
      p <: 0x34;
      p <: 0x56;
      p <: 0x78;
      p <: 0x9a;
    }
    {
      q when pinseq(0x34) :> tmp @ t;
    }
  }
  if (tmp != 0x34129954)
    return 1;
  if (t != 6)
    return 2;
  return 0;
}
