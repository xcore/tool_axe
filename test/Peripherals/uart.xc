// RUN: xcc -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe --uart-rx port=0x10200,bitrate=115200

#include <xs1.h>
#include <print.h>
#include <stdlib.h>
#define BIT_RATE 115200
#define BIT_TIME 100000000 / BIT_RATE

out port TXD = XS1_PORT_1A;

static void sendByte(out port TXD, unsigned byte) {
  unsigned time;
  timer t;
  t :> time ;
  /* output start bit */
  TXD <: 0;
  time += BIT_TIME ;
  t when timerafter(time) :> void;
  /* output data bits */
  for (int i = 0; i < 8; i++) {
    TXD <: >> byte;
    time += BIT_TIME;
    t when timerafter(time) :> void;
  }
  /* output stop bit */
  TXD <: 1;
  time += BIT_TIME ;
  t when timerafter(time) :> void;
}

static void sendBytes(out port TXD, unsigned char bytes[], unsigned length)
{
  for (unsigned i = 0; i < length; i++) {
    sendByte(TXD, bytes[i]);
  }
}

int main()
{
  TXD <: 1;
  sendBytes(TXD, "Hello World!\n", 13);
  return 0;
}
