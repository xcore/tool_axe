// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x10000 0x10200

#include <xs1.h>
#include <stdlib.h>

out port TXD = XS1_PORT_1A;
in port RXD = XS1_PORT_1B;

int main()
{
  unsigned start;
  TXD <: 0 @ start;
  TXD @ start + 100 <: 1;
  par {
    {
      TXD @ start + 200 <: 1;
    }
    {
      unsigned x;
      RXD @ start + 150 :> x;
      if (x != 1)
        exit(1);
    }
  }
  return 0;
}
