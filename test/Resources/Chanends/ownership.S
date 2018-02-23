// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: axe %t1.xe
#include <xs1.h>

.globl main
main:
 getr r3, XS1_RES_TYPE_CHANEND
 getr r0, XS1_RES_TYPE_SYNC
 getst r1, res[r0]
 ldap r11, t0
 init t[r1]:pc, r11
 set t[r1]:r0, r3
 getst r2, res[r0]
 ldap r11, t1
 init t[r2]:pc, r11
 set t[r2]:r0, r3
 msync res[r0]
 msync res[r0]
 mjoin res[r0]
 freer res[r0]
 ldc r0, 0
 retsp 0

t0:
 // Setup events on chanend and wait.
 eeu res[r0]
 ldap r11, success
 setv res[r0], r11
 ssync
 waiteu

success:
 ssync

t1:
 ssync
 // These instructions shouldn't change the owner of the chanend.
 setd res[r0], r0
 out res[r0], r0
 // The out should trigger an event on the other thread.
 ssync
