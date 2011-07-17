// RUN: xcc -target=XC-5 -O2 %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x10000 0x10300 --loopback 0x10200 0x10100

#include <xs1.h>
#include <print.h>
#include <stdlib.h>

struct port_pair {
	port first;
	port second;
};

struct port_pair a = {
	XS1_PORT_1A,
	XS1_PORT_1B
};

struct port_pair b = {
	XS1_PORT_1C,
	XS1_PORT_1D
};

unsigned char getByte()
{
	return 0x57;
}

void sender(struct port_pair &p) {
	timer t;
	unsigned time;
	unsigned char value;
	unsigned first_value = 0, second_value = 0;
	value = getByte();
	t :> time;
	for (unsigned i = 0; i < 8; i++) {
		t when timerafter(time+=100) :> void;
		if (value & 1) {
			second_value = !second_value;
			p.second <: second_value;
		} else {
			first_value = !first_value;
			p.first <: first_value;
		}
		value >>= 1;
	}
}

void putByte(unsigned char b)
{
  if (b != 0x57)
    exit(1);
  exit(0);
}

void receiver(struct port_pair &p)
{
	// Transition on A == 0
	// Transition on B == 1
	unsigned first_val = 0, second_val = 0;
	unsigned char value;
	for (unsigned i = 0; i < 8; i++) {
		select {
		case p.first when pinsneq(first_val) :> first_val:
			value >>= 1;
			break;
		case p.second when pinsneq(second_val) :> second_val:
			value >>= 1;
			value |= (1 << 7);
			break;
		}
	}
	putByte(value);
}

int main()
{
	a.first <: 0;
	a.second <: 0;
	par {
		sender(a);
		receiver(b);
	}
	return 0;
}
