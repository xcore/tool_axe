// RUN: xcc -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe

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

