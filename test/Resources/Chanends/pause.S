// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: axe %t1.xe
#include <xs1.h>

.text
.align 2
.globl main
main:
  getr r0, XS1_RES_TYPE_CHANEND
  getr r1, XS1_RES_TYPE_CHANEND
  getr r2, XS1_RES_TYPE_CHANEND
  setd res[r1], r0
  setd res[r2], r0

  out res[r1], r1
  outct res[r1], XS1_CT_PAUSE

  in r3, res[r0]
  eq r3, r3, r1
  ecallf r3

  out res[r2], r2
  outct res[r2], XS1_CT_END

  in r3, res[r0]
  chkct res[r0], XS1_CT_END
  eq r3, r3, r2
  ecallf r3

  outct res[r1], XS1_CT_END
  chkct res[r0], XS1_CT_END

  freer res[r0]
  freer res[r1]
  freer res[r2]

  ldc r0, 0
  retsp 0
