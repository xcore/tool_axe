// RUN: xcc -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe

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

