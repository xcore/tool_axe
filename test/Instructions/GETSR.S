// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: %sim %t1.xe
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe -mno-dual-issue
// RUN: %sim %t1.xe

.text
.globl main

main:
  getsr r11, 0xFFFF 
  eq r11, r11, 0
  ecallf r11
  
  setsr 0x1
  setsr 0x2
  getsr r11, 0xFFFF
  eq r11, r11, 3
  ecallf r11

  ldc r0, 0
  retsp 0

