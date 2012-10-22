//RUN: xcc -O2 %s -x xc %s -target=XK-1A -o %t1.xe
//RUN: axe %t1.xe
#ifdef __XC__
#include <stdlib.h>

void source(streaming chanend c)
{
  while (1)
    c <: 0;
}

void sink(streaming chanend c)
{
  for (unsigned i = 0; i < 100; i++)
    c :> int;
  exit(0);
}


void buffer(int buf[], unsigned bufsize_log2, streaming chanend a,
            streaming chanend b);

#define SIZE_LOG_2 5

int main()
{
  streaming chan a, b;
  int array[1 << SIZE_LOG_2];
  par {
    source(a);
    buffer(array, SIZE_LOG_2, a, b);
    sink(b);
  }
  return 0;
}
#else
#include <xs1.h>

//r0 - buf
//r1 - mask
//r2 - inChan
//r3 - outChan
//r4 - read
//r5 - write
//r6 - nextWrite
//r10 - tmp1
//r11 - tmp2

.align 4
interrupt:
  in r11, res[r2]
  stw r11, r0[r5]
  mov r5, r6
  add r6, r5, 1
  and r6, r6, r1
  eq r11, r4, r6
  eet r11, res[r2]
  kret

.globl buffer
.align 4
buffer:
  mkmsk r1, r1
  ldc r4, 0
  ldc r5, 0
  ldc r6, 1
  ldap r11, interrupt
  setv res[r2], r11
  setc res[r2], XS1_SETC_IE_MODE_INTERRUPT
  setsr XS1_SR_IEBLE_SET(0, 1)

input:
  in r11, res[r2]
  stw r11, r0[r5]
  mov r5, r6
  add r6, r5, 1
  and r6, r6, r1

output:
  add r11, r4, 1
  ldw r10, r0[r4]
  and r4, r11, r1
  eeu res[r2]
  out res[r3], r10
  edu res[r2]
  eq r10, r4, r5
  bf r10, output
  bu input
.globl buffer.nstackwords
.set buffer.nstackwords, 0
#endif
