#include <xs1.h>
#define KERNEL_STACK_SIZE 4

.section .ctors, "aw", @progbits
.align 4
.word init

.section .dp.data, "awd", @progbits
.align 4
kernel_stack:
.space KERNEL_STACK_SIZE * 8 * 4
expected_et:
.word 0
expected_ed:
.word 0

.text
.align 2
init:
  entsp 1
  getr r0, XS1_RES_TYPE_SYNC
  ldap r11, init_thread
  getst r1, res[r0]
loop:
  init t[r1]:pc, r11
  getst r1, res[r0]
  bt r1, loop
  msync res[r0]
  bl init_ksp
  bl init_kep
  mjoin res[r0]
  freer res[r0]
  retsp 1

init_ksp:
  ldc r1, KERNEL_STACK_SIZE
  get r11, id
  mul r1, r1, r11
  ldaw r11, dp[kernel_stack]
  ldaw r11, r11[r1]
  ldaw r1, sp[0]
  set sp, r11
  stw r1, sp[0]
  krestsp 0
  retsp 0

init_kep:
  ldap r11, exception_handler
  set kep, r11
  retsp 0

init_thread:
  bl init_ksp
  bl init_kep
  ssync

.align 128
exception_handler:
  kentsp 4
  stw r11, sp[0]
  stw spc, sp[1]
  stw r0, sp[2]

  // Check if the exception is expected.
  get r11, et
  ldw r0, dp[expected_et]
  eq r11, r0, r11
  bf r11, fail

  get r11, ed
  ldw r0, dp[expected_ed]
  eq r11, r0, r11
  bf r11, fail

  // Reset expected exception type.
  ldc r0, 0
  stw r0, dp[expected_et]

  // Increment spc.
  ldw r11, sp[1]
  ld16s r0, r11[r0]
  add r11, r11, 2
  ashr r0, r0, 8
  ashr r0, r0, 4
  add r0, r0, 1
  bt r0, skip_long_instruction
  add r11, r11, 2
skip_long_instruction:
  stw r11, sp[1]

  // Return.
  ldw r11, sp[0]
  ldw spc, sp[1]
  ldw r0, sp[2]
  krestsp 4
  kret

.globl exception_expect
exception_expect:
  stw r0, dp[expected_et]
  stw r1, dp[expected_ed]
  retsp 0

.globl exception_check
exception_check:
  ldw r0, dp[expected_et]
  bt r0, fail
retsp 0

fail:
  ldc r0, 0 // OSCALL_EXIT
  ldc r1, 1
  bu _DoSyscall
  clre
  waiteu
