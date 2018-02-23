// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback PORT_8A PORT_8B --loopback PORT_8A PORT_8C
// RUN: xcc -O2 -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: axe %t1.xe --loopback PORT_8A PORT_8B --loopback PORT_8A PORT_8C

#include <xs1.h>
#include <stdlib.h>

port p1 = XS1_PORT_8A;
port p2 = XS1_PORT_8B;
port p3 = XS1_PORT_8C;

int main()
{
  int x, y;
  p1 <: 0x5e;
  sync(p1);
  p2 :> x;
  p3 :> y;
  if (x != 0x5e)
    _Exit(1);
  if (y != 0x5e)
    _Exit(1);
  return 0;
}
