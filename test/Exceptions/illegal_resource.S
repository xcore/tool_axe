// RUN: xcc -target=XC-5 %s %exception_expect -o %t1.xe
// RUN: axe %t1.xe
#include <xs1.h>
.section .dp.data,"awd",@progbits
.align 4
p:
.word XS1_PORT_1A

.text
.globl main
.align 2
main:
  entsp 1
  ldw r4, dp[p]
  setc res[r4], XS1_SETC_INUSE_ON
  ldc r0, XS1_ET_ILLEGAL_RESOURCE
  mov r1, r4
  bl exception_expect
  setc res[r4], \
    XS1_SETC_LMODE_SET(0, XS1_SETC_LMODE_PIN_DELAY) | \
    XS1_SETC_MODE_SET(0, XS1_SETC_MODE_LONG) | \
    XS1_SETC_VALUE_SET(0, 6)
  bl exception_check
  ldc r0, 0
  retsp 1
