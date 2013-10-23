// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: axe --args %t1.xe hello world

.globl main

.align 2
main:
  entsp 1
  ldc r0, 13
  ldaw r1, dp[_buffer]
  ldc r2, 200
  bl _DoSyscall
  eq r0, r0, 3  // argc=3
  ecallf r0
  ldaw r4, dp[_buffer]
  eq r0, r1, r4 // argv[] at start of buffer
  ecallf r0

  ldw r0, r4[1] // argv[1]
  ldaw r1, dp[_arg1]
  bl strcmp
  ecallt r0

  ldw r0, r4[2] // argv[2]
  ldaw r1, dp[_arg2]
  bl strcmp
  ecallt r0

  ldc r0, 0
  retsp 1

.section .dp.data, "awd", @progbits
.align 4
_buffer:
.space 800

.align 4
_arg1:
.asciiz "hello"

.align 4
_arg2:
.asciiz "world"
