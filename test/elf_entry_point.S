// RUN: xcc -nostdlib -target=XK-1A %s -o %t1.xe
// RUN: %sim %t1.xe
.text
.align 2
  ldc r0, 1
  bu fin
.globl _start
_start:
  ldc r0, 0
  bu fin

.globl _DoSyscall
_DoSyscall:
  retsp 0

fin:
  mov r1, r0
  ldc r0, 0 //OSCALL_EXIT
  bu _DoSyscall
