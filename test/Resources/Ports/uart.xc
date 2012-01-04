// RUN: xcc -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe --loopback XS1_PORT_1A XS1_PORT_1B

#include <xs1.h>
#include <print.h>
#include <stdlib.h>
#define BIT_RATE 115200
#define BIT_TIME 100000000 / BIT_RATE

out port TXD = XS1_PORT_1A;
in port RXD = XS1_PORT_1B;

unsigned char getByte()
{
  return 0xa3;
}

void transmitter (out port TXD) {
  unsigned byte, time;
  timer t;
  /* get next byte to transmit */
  byte = getByte();
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

void putByte(unsigned char b)
{
  if (b != 0xa3)
    exit(1);
  exit(0);
}

void receiver(in port RXD) {
  unsigned byte, time;
  timer t;
  /* wait for start bit */
  RXD when pinseq(0) :> void;
  t :> time;
  time += BIT_TIME / 2;
  /* input data bits */
  for (int i = 0; i < 8; i++) {
    time += BIT_TIME;
    t when timerafter(time) :> void;
    RXD :> >> byte;
  }
  /* input stop bit */
  time += BIT_TIME;
  t when timerafter(time) :> void;
  RXD :> void;
  putByte(byte >> 24);
}


int main()
{
  par {
    transmitter(TXD);
    receiver(RXD);
  }
  return 0;
}
