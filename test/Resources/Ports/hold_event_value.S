// RUN: xcc -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x10000 0x10200
#include <xs1.h>

.section .dp.data, "awd", @progbits
.align 4
p:
.word XS1_PORT_1A
q:
.word XS1_PORT_1B
c:
.word XS1_CLKBLK_1

.globl main
main:
  entsp 4
  stw r4, sp[1]
  stw r5, sp[2]
  stw r6, sp[3]

  ldw r4, dp[p]
  setc res[r4], XS1_SETC_INUSE_ON
  ldw r5, dp[q]
  setc res[r5], XS1_SETC_INUSE_ON
  ldc r11, 0
  out res[r5], r11
  ldw r6, dp[c]
  setc res[r6], XS1_SETC_INUSE_ON
  mov r0, r4
  mov r1, r6
  bl configure_in_port

  mov r0, r6
  ldc r1, 10
  bl configure_clock_ref

  ldap r11, event
  setv res[r4], r11
  eeu res[r4]
  setsr XS1_SR_EEBLE_SET(0, 1)

  setc res[r6], XS1_SETC_RUN_STARTR
  waiteu

event:
  ldc r11, 1
  out res[r5], r11
  getr r1, XS1_RES_TYPE_TIMER
  in r11, res[r1]
  ldc r0, 100
  add r11, r11, r0
  setc res[r1], XS1_SETC_COND_AFTER
  setd res[r1], r11
  in r11, res[r1]
  getts r0, res[r4]
  ecallt r0
  in r0, res[r4]
  ecallt r0
  ldw r4, sp[1]
  ldw r5, sp[2]
  ldw r6, sp[3]
  retsp 4