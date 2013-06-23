// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --warn-packet-overtake
#include <xs1.h>

int main() {
  chan c;
  par {
    {
      outuchar(c, 1);
      outct(c, XS1_CT_END);
      outuchar(c, 2);
      outct(c, XS1_CT_END);
    }
    {
      inuchar(c);
      chkct(c, XS1_CT_END);
      inuchar(c);
      chkct(c, XS1_CT_END);
    }
  }
  return 0;
}
