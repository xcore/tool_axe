// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: %sim %t1.xe
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: %sim %t1.xe

.text
.globl main

foo:
  retsp 0

bar:
  retsp 0

baz:
  retsp 0

main:
  entsp 1
  bla cp[1]
  bla cp[2]
  bla cp[3]
  ldc r0, 0
  retsp 1

.section .cp.rodata, "ac", @progbits
.align 4
.word 0
.word foo
.word bar
.word baz

