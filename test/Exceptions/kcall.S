// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: %sim %t1.xe
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: %sim %t1.xe
#include <xs1.h>

.text
.globl main
.align 2
main:
  ldap r11, exception_handler
  set kep, r11

  ldc r0, 5
  .align 4
  kcall 5
  .align 4
  bu kcall2
  bu fail
  bu fail
  bu fail
  bu fail
  bu fail

kcall2:
  ldc r0, 0xff
  .align 4
  kcall 0xff
  .align 4
  bu kcall3
  bu fail
  bu fail
  bu fail
  bu fail
  bu fail

kcall3:
  ldc r0, 5
  .align 4
  kcall r0
  .align 4
  bu fin
  bu fail
  bu fail
  bu fail
  bu fail
  bu fail

fin:
  retsp 0

.align 128
exception_handler:
  bu fail
.align 64
kernel_entry:
  get r11, et
  ldc r1, 15
  eq r11, r11, r1
  bf r11, fail

  get r11, ed
  eq r11, r11, r0
  bf r11, fail
  ldc r0, 0

  ldaw r3, sp[0]
  extsp 1
  stw spc, sp[1]
  ldw r1, sp[1]
  add r1, r1, 4
  stw r1, sp[1]
  ldw spc, sp[1]
  set sp, r3

  kret

fail:
  ldc r0, 0 // OSCALL_EXIT
  ldc r1, 1
  bu _DoSyscall
  clre
  waiteu
