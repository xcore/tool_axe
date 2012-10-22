// RUN: xcc -target=XK-1A %s %exception_expect -o %t1.xe
// RUN: axe %t1.xe
#include <xs1.h>

.text
.globl main
.align 2
main:
  entsp 1
  // Unsigned operations.
  // 7 / 0
  ldc r0, 7
  ldc r1, 0
  bl exception_expect
  ldc r1, 10
  ldc r11, 0
  divu r0, r1, r11

  bl exception_check
  // 7 % 0
  ldc r0, 7
  ldc r1, 0
  bl exception_expect
  ldc r1, 10
  ldc r11, 0
  remu r0, r1, r11

  bl exception_check
  // 0x000000005 / 0
  ldc r0, 7
  ldc r1, 0
  bl exception_expect
  ldc r2, 0
  ldc r3, 5
  ldc r11, 0
  ldivu r0, r1, r2, r3, r11

  bl exception_check
  // 0x700000005 / 0x7
  ldc r0, 7
  ldc r1, 0
  bl exception_expect
  ldc r2, 7
  ldc r3, 5
  ldc r11, 7
  ldivu r0, r1, r2, r3, r11

  // Signed operations.

  // 7 / 0
  ldc r0, 7
  ldc r1, 0
  bl exception_expect
  ldc r1, 10
  ldc r11, 0
  divs r0, r1, r11
  bl exception_check

  // MIN_INT / -1
  ldc r0, 7
  ldc r1, 0
  bl exception_expect
.section .cp.rodata, "ac", @progbits
.align 4
const1:
.word 0x80000000
.text
  ldw r1, cp[const1]
  mkmsk r11, 32
  divs r0, r1, r11
  bl exception_check

  // 7 % 0
  ldc r0, 7
  ldc r1, 0
  bl exception_expect
  ldc r1, 10
  ldc r11, 0
  rems r0, r1, r11
  bl exception_check

  // MIN_INT % -1
  ldc r0, 7
  ldc r1, 0
  bl exception_expect
  ldw r1, cp[const1]
  mkmsk r11, 32
  rems r0, r1, r11
  bl exception_check

  ldc r0, 0
  retsp 1
