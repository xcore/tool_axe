/*
 * RUN: xcc -O2 -target=XCORE-200-EXPLORER %s -o %t1.xe
 * RUN: %sim %t1.xe
 */
#include <stdio.h>

int main(void) {
    int x, y;
    asm("{ldap r11, main; nop}\n add %0, r11, 0" : "=r" (x));
    asm("{nop; ldap r11, main}\n add %0, r11, 0" : "=r" (y));

    printf("%08x %08x\n", x, y);

    if (x == y) {
        return 0;
    } else { 
        return 1;
    }
}
