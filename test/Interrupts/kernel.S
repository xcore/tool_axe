//RUN: xcc -O2 %s -x xc %s -target=XK-1A -o %t1.xe
//RUN: axe %t1.xe

// To test LD/ST{SPC,SSR,SED,ET}

#ifdef __XC__
#include <stdlib.h>

void bar(chanend c);

void foo(chanend c) 
{
  c <: 0xABC;
}

int main()
{
  chan c;
  par {
    foo(c);
    bar(c);
  }
  return 0;
}

#else
#include <xs1.h>

.text
.globl bar
.align 4
bar:
  
  // Set interrupts on the channel
  ldap r11, kernel
  setv res[r0], r11
  setc res[r0], XS1_SETC_IE_MODE_INTERRUPT
  eeu  res[r0]
  setsr XS1_SR_IEBLE_SET(0, 1)

  // Wait for an interrupt
  waiteu

  retsp 0
.globl bar.nstackwords
.set bar.nstackwords, 5

.align 4
kernel:
  entsp 5
  stw spc, sp[1]
  stw ssr, sp[2]
  stw sed, sp[3]
  stw et,  sp[4]
  chkct res[r0], XS1_CT_END
  outct res[r0], XS1_CT_END
  in r11, res[r0]
  chkct res[r0], XS1_CT_END
  outct res[r0], XS1_CT_END
  ldw spc, sp[1]
  ldw ssr, sp[2]
  ldw sed, sp[3]
  ldw et,  sp[4]
  retsp 5
  kret

#endif

