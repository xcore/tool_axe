// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: axe --args %t1.xe hello world
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: axe --args %t1.xe hello world

.globl main

.set buflen, (_buffer_end - _buffer)

.align 2
main:
  entsp 1

  // System call should return -2 if the buffer is too small.
  ldc r0, 13
  ldaw r1, dp[_buffer]
  ldc r2, 4
  bl _DoSyscall
  add r0, r0, 2
  ecallt r0

  // Otherwise it should return argc and populate the buffer with the command
  // line args.
  ldc r0, 13
  ldaw r1, dp[_buffer]
  ldc r2, buflen
  bl _DoSyscall

  eq r0, r0, 3  // argc=3
  ecallf r0

  ldaw r4, dp[_buffer]
  eq r0, r1, r4 // argv[] is at start of buffer
  ecallf r0
  ldc r5, 4*4	// sizeof(argv[]) including null terminator
  add r5, r4, r5 // next free byte =  &argv[argc+1]

  ldw r0, r4[0] // argv[0]
  eq r1, r0, r5 //   = next free byte
  ecallf r1
  bl strlen	// strlen(argv[0])
  add r5, r5, r0
  add r5, r5, 1	// next free byte

  ldw r0, r4[1] // argv[1]
  eq r1, r0, r5 //   = next free byte
  ecallf r1
  ldaw r1, dp[_arg1]
  bl strcmp	// argv[1] == "hello"
  ecallt r0
  ldw r0, r4[1] // argv[1]
  bl strlen     // strlen(argv[1])
  add r5, r5, r0
  add r5, r5, 1 // next free byte

  ldw r0, r4[2] // argv[2]
  eq r1, r0, r5 //   = next free byte
  ecallf r1
  ldaw r1, dp[_arg2]
  bl strcmp     // argv[2] == "world"
  ecallt r0

  ldw r0, r4[3] // argv[argc] = null
  ldc r6, 8
  ecallt r0

  ldc r0, 0
  retsp 1

.section .dp.data, "awd", @progbits
.align 4
_buffer:
.space 400
_buffer_end:

.align 4
_arg1:
.asciiz "hello"

.align 4
_arg2:
.asciiz "world"
