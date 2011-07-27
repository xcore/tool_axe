// RUN: xcc -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe
#include <xs1.h>

.globl main
.text
.align 2
main:
  getr r0, XS1_RES_TYPE_THREAD
  getr r1, XS1_RES_TYPE_CHANEND
  getr r2, XS1_RES_TYPE_CHANEND
  setd res[r2], r1
  ldap r11, thread
  init t[r0]:pc, r11
  set t[r0]:r0, r2
  start t[r0]
  chkct res[r1], XS1_CT_END
  freer res[r1]
  freer res[r2]
  ldc r0, 0
  retsp 0

.align 2
thread:
  outct res[r0], XS1_CT_END
  freet
