// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback PORT_4A PORT_8B[0:4] --loopback PORT_1A PORT_8B[4] --loopback PORT_1B PORT_8B[5] --loopback PORT_1C PORT_8B[6] --loopback PORT_1D PORT_8B[7]
// RUN: xcc -O2 -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: axe %t1.xe --loopback PORT_4A PORT_8B[0:4] --loopback PORT_1A PORT_8B[4] --loopback PORT_1B PORT_8B[5] --loopback PORT_1C PORT_8B[6] --loopback PORT_1D PORT_8B[7]
#include <xs1.h>
#include <stdlib.h>

#define VERIFY(x) do { if (!(x)) _Exit(1); } while(0)

port p = XS1_PORT_8B;
port p0_4 = XS1_PORT_4A;
port p4 = XS1_PORT_1A;
port p5 = XS1_PORT_1B;
port p6 = XS1_PORT_1C;
port p7 = XS1_PORT_1D;

int main() {
  int tmp;
  p <: 0b01001010;
  sync(p);

  p0_4 :> tmp;
  VERIFY(tmp == 0b1010);
  p4 :> tmp;
  VERIFY(tmp == 0);
  p5 :> tmp;
  VERIFY(tmp == 0);
  p6 :> tmp;
  VERIFY(tmp == 1);
  p7 :> tmp;
  VERIFY(tmp == 0);

  p :> void;
  p0_4 <: 0b1001;
  p4 <: 0b0;
  p5 <: 0b1;
  p6 <: 0b1;
  p7 <: 0b1;
  sync(p7);
  p :> tmp;
  VERIFY(tmp == 0b11101001);

  return 0;
}
