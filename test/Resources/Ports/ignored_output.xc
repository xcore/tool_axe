// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x10000 0x10200
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: axe %t1.xe --loopback 0x10000 0x10200

#include <xs1.h>

port p = XS1_PORT_1A;
port q = XS1_PORT_1B;

int main()
{
	for (unsigned i = 0; i < 1000; i++) {
		p <: 0;
		p <: 1;
	}
	return 0;
}
