// RUN: xcc -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe
.text
.globl main
.align 2
main:
  entsp 1
  ldaw r11, cp[table]
  ldc r0, 2
  blat 1
  blat 2
  retsp 1

a:
  eq r1, r0, 2
  ecallf r0
  sub r0, r0, 1
  retsp 0

b:
  eq r1, r0, 1
  ecallf r0
  sub r0, r0, 1
  retsp 0

.section .cp.rodata, "ac", @progbits
.align 4
table:
.word 0
.word a
.word b
