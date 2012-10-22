// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe
.section .cp.rodata, "ac", @progbits
.align 4
.word 0
cp_word:
.word -5

.section .dp.data, "awd", @progbits
.align 4
.word 0
dp_word:
.word -5
dp_word2:
.word 0
dp_arr:
.word 0
.word 0x01234567
.word 0x7aaa8043
.word 0x53bc128a

.text
.globl main
main:
  // CP relative loads.
  LDWCP_ru6 r0, cp[cp_word]
  add r0, r0, 5
  ecallt r0
  LDWCP_lru6 r0, cp[cp_word]
  add r0, r0, 5
  ecallt r0
  LDWCPL_u10 r11, cp[cp_word]
  add r0, r11, 5
  ecallt r0
  LDWCPL_lu10 r11, cp[cp_word]
  add r0, r11, 5
  ecallt r0

  // DP relative loads / stores.
  LDWDP_ru6 r0, dp[dp_word]
  add r0, r0, 5
  ecallt r0
  LDWDP_lru6 r0, dp[dp_word]
  add r0, r0, 5
  ecallt r0
  
  sub r0, r0, 11
  STWDP_ru6 r0, dp[dp_word]
  STWDP_lru6 r0, dp[dp_word2]
  LDWDP_ru6 r0, dp[dp_word]
  add r0, r0, 11
  ecallt r0
  LDWDP_lru6 r0, dp[dp_word]
  add r0, r0, 11
  ecallt r0

  // Register loads.
  ldaw r2, dp[dp_arr]

  ldw r0, r2[2]
.section .cp.rodata, "ac", @progbits
expected1:
.word 0x7aaa8043
.text
  ldw r1, cp[expected1]
  eq r0, r0, r1
  ecallf r0

  ldc r3, 1
  ldw r0, r2[r3]
.section .cp.rodata, "ac", @progbits
expected2:
.word 0x01234567
.text
  ldw r1, cp[expected2]
  eq r0, r0, r1
  ecallf r0

  ldc r3, 4
  ld16s r0, r2[r3]
.section .cp.rodata, "ac", @progbits
expected3:
.word 0xffff8043
.text
  ldw r1, cp[expected3]
  eq r0, r0, r1
  ecallf r0

  ldc r3, 5
  ld16s r0, r2[r3]
.section .cp.rodata, "ac", @progbits
expected4:
.word 0x00007aaa
.text
  ldw r1, cp[expected4]
  eq r0, r0, r1
  ecallf r0

  ldc r3, 9
  ld8u r0, r2[r3]
.section .cp.rodata, "ac", @progbits
expected5:
.word 0x00000080
.text
  ldw r1, cp[expected5]
  eq r0, r0, r1
  ecallf r0

  // Register stores.
.section .cp.rodata, "ac", @progbits
const1:
.word 0x864123ab
const2:
.word 0xbc471ef6
const3:
.word 0x819fefcb
const4:
.word 0xbcef123f
.text
  ldw r1, cp[const1]
  stw r1, r2[1]
  ldw r1, cp[const2]
  ldc r3, 2
  stw r1, r2[r3]
  ldw r1, cp[const3]
  ldc r3, 7
  st16 r1, r2[r3]
  ldw r1, cp[const4]
  ldc r3, 13
  st8 r1, r2[r3]

  ldw r1, cp[const1]
  ldw r0, dp[dp_arr+4]
  eq r0, r0, r1
  ecallf r0

  ldw r1, cp[const2]
  ldw r0, dp[dp_arr+8]
  eq r0, r0, r1
  ecallf r0

.section .cp.rodata, "ac", @progbits
expected6:
.word 0xefcb3f8a
.text
  ldw r1, cp[expected6]
  ldw r0, dp[dp_arr+12]
  eq r0, r0, r1
  ecallf r0

  ldc r0, 0
  retsp 0
