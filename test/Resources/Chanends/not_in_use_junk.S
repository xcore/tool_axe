// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: %sim %t1.xe
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: %sim %t1.xe
#include <xs1.h>
.text
.align 2
.globl main
main:
  getr r0, XS1_RES_TYPE_CHANEND
  ldc r11, 0x100
  add r1, r0, r11
  setd res[r0], r1
  // This should be junked since the destination is not in use.
  out res[r0], r0
  out res[r0], r0
  getr r2, XS1_RES_TYPE_CHANEND
  eq r11, r1, r2
  ecallf r11
  // This should still be junked as we haven't ended the previous packet.
  out res[r0], r0
  out res[r0], r0
  outct res[r0], XS1_CT_END
  // This message should be received.
  out res[r0], r11
  outct res[r0], XS1_CT_END
  in r2, res[r1]
  eq r11, r2, r11
  ecallf r11
  chkct res[r1], XS1_CT_END
  freer res[r0]
  freer res[r1]
  ldc r0, 0
  retsp 0
