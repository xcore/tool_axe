// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: %sim %t1.xe
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: %sim %t1.xe
#include <xs1.h>

.section .cp.rodata, "ac", @progbits
.align 4
const1:
.word 0x3a
const2:
.word 0x1e761921

.text
.align 2
.globl main
main:
  getr r0, XS1_RES_TYPE_CHANEND
  getr r1, XS1_RES_TYPE_CHANEND
  setd res[r0], r1
  setd res[r1], r0
  getd r2, res[r0]
  eq r2, r2, r1
  ecallf r2
  ldw r11, cp[const1]
  outt res[r0], r11
  testct r11, res[r1]
  ecallt r11
  int r2, res[r1]
  ldw r11, cp[const1]
  eq r11, r2, r11
  ecallf r11

  ldw r11, cp[const1]
  outct res[r0], r11
  testct r11, res[r1]
  ecallf r11
  testwct r11, res[r1]
  eq r11, r11, 1
  ecallf r11
  inct r2, res[r1]
  ldw r11, cp[const1]
  eq r11, r2, r11
  ecallf r11

  ldw r11, cp[const2]
  out res[r0], r11
  testct r11, res[r1]
  ecallt r11
  testwct r11, res[r1]
  ecallt r11
  in r2, res[r1]
  ldw r11, cp[const2]
  eq r11, r2, r11
  ecallf r11

  ldc r0, 0
  retsp 0
