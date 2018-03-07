/*
 * RUN: xcc -target=XK-1A %s -o %t1.xe
 * RUN: %sim %t1.xe
 * RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
 * RUN: %sim %t1.xe
 */

.section .dp.data, "awd", @progbits
.align 4
ldc1:
ldc r0, 1

.text
.globl main
.align 2
f:
  nop
patch_address:
  ldc r0, 0
  bt r0, end
  ldaw r11, dp[ldc1]
  ld16s r1, r11[r0]
  ldap r11, patch_address
  st16 r1, r11[r0]
end:
  retsp 0

main:
  entsp 1
  bl f
  ecallt r0
  bl f
  eq r0, r0, 1
  ecallf r0
  ldc r0, 0
  retsp 1
