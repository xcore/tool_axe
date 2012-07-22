// RUN: xcc -O2 -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe --loopback PORT_1A PORT_1B

#include <xs1.h>
#include <stdlib.h>
#define VERIFY(x) do { if(!(x)) { _Exit(1); }} while(0)

port p = XS1_PORT_1A;
port q = XS1_PORT_1B;

int main() {
  int x;
  set_port_inv(p);
  // Check driven data is inverted.
  p <: 0;
  sync(p);
  q :> x;
  q :> x;
  VERIFY(x == 1);
  p <: 1;
  sync(p);
  q :> x;
  q :> x;
  VERIFY(x == 0);

  p :> int;
  // Check sampled data is inverted.
  q <: 0;
  sync(q);
  p :> x;
  p :> x;
  VERIFY(x == 1);
  q <: 1;
  sync(q);
  p :> x;
  p :> x;
  VERIFY(x == 0);
 
  // Check peek returns inverted data.
  VERIFY(peek(p) == 0);

  // Check set_port_no_inv disables the inversion.
  set_port_no_inv(p);
  p :> x;
  p :> x;
  VERIFY(x == 1);
  return 0;
}
