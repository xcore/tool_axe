// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: %sim %t1.xe
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: %sim %t1.xe

.globl main

.align 2
main:
  entsp 1
  ldc r0, 99
  bl _DoSyscall
  eq r0, r0, 1
  ecallf r0
  ldc r0, 0
  retsp 1
