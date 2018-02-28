// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: axe %t1.xe
#include <xs1.h>

.text
.globl main
.align 2
main:
  ldap r11, exception_handler
  set kep, r11

  ldc r0, 5
  kcall 5
  bt r0, fail

  ldc r0, 0xffff
  kcall 0xffff
  bt r0, fail

  ldc r0, 5
  kcall r0
  bt r0, fail

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
  kret

fail:
  ldc r0, 0 // OSCALL_EXIT
  ldc r1, 1
  bu _DoSyscall
  clre
  waiteu
