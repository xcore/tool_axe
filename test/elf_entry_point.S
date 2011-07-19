// RUN: xcc -nostdlib -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe
.text
.align 2
  ldc r0, 1
  bu exit
.globl _start
_start:
  ldc r0, 0
  bu exit

.globl _DoSyscall
_DoSyscall:
  retsp 0

exit:
  mov r1, r0
  ldc r0, 0 //OSCALL_EXIT
  bu _DoSyscall
