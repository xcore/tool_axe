#include <xs1.h>
#include <stdlib.h>

// xsim a.xe --plugin LoopbackPort.so '-port stdcore[0] XS1_PORT_1A 1 0 -port stdcore[0] XS1_PORT_1E 1 0 -port stdcore[0] XS1_PORT_1B 1 0 -port stdcore[0] XS1_PORT_1G 1 0 -port stdcore[0] XS1_PORT_1C 1 0 -port stdcore[0] XS1_PORT_1F 1 0 -port stdcore[0] XS1_PORT_1D 1 0 -port stdcore[0] XS1_PORT_1H 1 0 -port stdcore[0] XS1_PORT_1I 1 0 -port stdcore[0] XS1_PORT_1M 1 0 -port stdcore[0] XS1_PORT_1J 1 0 -port stdcore[0] XS1_PORT_1O 1 0 -port stdcore[0] XS1_PORT_1K 1 0 -port stdcore[0] XS1_PORT_1N 1 0 -port stdcore[0] XS1_PORT_1L 1 0 -port stdcore[0] XS1_PORT_1P 1 0'
// axe a.xe --loopback 0x10200 0x10600 --loopback 0x10000 0x10500 --loopback 0x10100 0x10400 --loopback 0x10300 0x10700 --loopback 0x10a00 0x10c00 --loopback 0x10800 0x10e00 --loopback 0x10900 0x10d00 --loopback 0x10b00 0x10f00

// XS1_PORT_1A <-> XS1_PORT_1E
// XS1_PORT_1B <-> XS1_PORT_1G
// XS1_PORT_1C <-> XS1_PORT_1F
// XS1_PORT_1D <-> XS1_PORT_1H
// XS1_PORT_1I <-> XS1_PORT_1M
// XS1_PORT_1J <-> XS1_PORT_1O
// XS1_PORT_1K <-> XS1_PORT_1N
// XS1_PORT_1L <-> XS1_PORT_1P

// 0x10200 <-> 0x10600
// 0x10000 <-> 0x10500
// 0x10100 <-> 0x10400
// 0x10300 <-> 0x10700
// 0x10a00 <-> 0x10c00
// 0x10800 <-> 0x10e00
// 0x10900 <-> 0x10d00
// 0x10b00 <-> 0x10f00

struct portStruct {
  out buffered port:32 p;
  in port pReadyIn;
  out port pReadyOut;
  out port pClkOut;
  clock pClk;
  in buffered port:32 q;
  in port qReadyIn;
  out port qReadyOut;
  in port qClkIn;
  clock qClk;
};

struct portStruct s[] = {
  {
    XS1_PORT_1A,
    XS1_PORT_1B,
    XS1_PORT_1C,
    XS1_PORT_1D,
    XS1_CLKBLK_1,
    XS1_PORT_1E,
    XS1_PORT_1F,
    XS1_PORT_1G,
    XS1_PORT_1H,
    XS1_CLKBLK_2
  },
  {
    XS1_PORT_1I,
    XS1_PORT_1J,
    XS1_PORT_1K,
    XS1_PORT_1L,
    XS1_CLKBLK_3,
    XS1_PORT_1M,
    XS1_PORT_1N,
    XS1_PORT_1O,
    XS1_PORT_1P,
    XS1_CLKBLK_4
  }
};

static void test(struct portStruct &s, unsigned runs) {
  chan c;
  configure_clock_rate(s.pClk, 100, 8);
  configure_port_clock_output(s.pClkOut, s.pClk);
  configure_clock_src(s.qClk, s.qClkIn);
  configure_out_port_handshake(s.p, s.pReadyIn, s.pReadyOut, s.pClk, 0);
  configure_in_port_handshake(s.q, s.qReadyIn, s.qReadyOut, s.qClk);
  start_clock(s.qClk);
  start_clock(s.pClk);
  par {
    for (unsigned i = 0; i < runs; i++) {
      int tmp;
      s.p <: i;
      s.q :> tmp;
      c <: tmp;
    }
    for (unsigned i = 0; i < runs; i++) {
      int tmp;
      c :> tmp;
      if (tmp != i)
        _Exit(1);
    }
  }
}

#ifndef RUNS
#define RUNS 100
#endif

int main()
{
  par {
    test(s[0], RUNS);
    test(s[1], RUNS);
  }
  return 0;
}
