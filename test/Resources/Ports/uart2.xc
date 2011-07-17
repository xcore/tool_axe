// RUN: xcc -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x10000 0x10200

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
  /* get next byte to transmit */
  byte = getByte();
  /* output start bit */
  TXD <: 0 @ time;
  time += BIT_TIME ;
  /* output data bits */
  for (int i = 0; i < 8; i++) {
    TXD @ time <: >> byte;
    time += BIT_TIME;
  }
  /* output stop bit */
  TXD @ time <: 1;
  time += BIT_TIME ;
  TXD @ time <: 1;
}

void putByte(unsigned char b)
{
  if (b != 0xa3)
    exit(1);
  exit(0);
}

void receiver(in port RXD) {
  unsigned byte, time;
  /* wait for start bit */
  RXD when pinseq(0) :> void @ time;
  time += BIT_TIME / 2;
  /* input data bits */
  for (int i = 0; i < 8; i++) {
    time += BIT_TIME;
    RXD @ time :> >> byte;
  }
  /* input stop bit */
  time += BIT_TIME;
  RXD @ time :> void;
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
