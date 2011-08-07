// RUN: xcc -target=XC-5 %s %exception_expect -o %t1.xe
// RUN: axe %t1.xe
#include <xs1.h>

.globl main
.align 2
main:
  entsp 1
  ldc r0, 7
  ldc r1, 0
  bl exception_expect
  ldc r1, 10
  ldc r11, 0
  divs r0, r1, r11
  bl exception_check

  ldc r0, 0
  retsp 1
