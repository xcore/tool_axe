// RUN: xcc -target=XK-1A %s %exception_expect -o %t1.xe
// RUN: axe %t1.xe
// RUN: xcc -target=XCORE-200-EXPLORER %s %exception_expect -o %t1.xe
// RUN: axe %t1.xe
#include <xs1.h>

.text
.globl main
.align 2
main:
  entsp 3
  stw r4, sp[1]
  stw r5, sp[2]
  ldc r4, 0
  ldc r5, 1

  // ecallt
  ecallt r4
  ldc r0, 8
  ldc r1, 0
  bl exception_expect
  ecallt r5
  bl exception_check

  // ecallf
  ecallf r5
  ldc r0, 8
  ldc r1, 0
  bl exception_expect
  ecallf r4
  bl exception_check

  ldw r4, sp[1]
  ldw r5, sp[2]
  ldc r0, 0
  retsp 3
