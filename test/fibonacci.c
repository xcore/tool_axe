/*
 * RUN: xcc -target=XK-1A %s -o %t1.xe
 * RUN: axe %t1.xe
 * RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
 * RUN: axe %t1.xe
 */

///////////////////////////////////////////////////////////
//recursion
///////////////////////////////////////////////////////////

#include "stdlib.h"
#include "stdio.h"

///////////////////////////////////////////////////////////
//Main
///////////////////////////////////////////////////////////

int main(void) 
{
    int fibonacci_depth = 80;  		// The number of fibonacci numbers we will print 
    int i = 0;        				// The index of fibonacci number to be printed next
    unsigned long long current = 1; // The value of the (i)th fibonacci number
    unsigned long long next = 1;    // The value of the (i+1)th fibonacci number
    unsigned long long twoaway = 0; // The value of the (i+2)th fibonacci number

	printf("\tI \t Fibonacci(I) \n\t=====================\n");
 
	for (i=1; i<=fibonacci_depth; i++) 
	{
		printf("\t%d \t   %llu\n", i, current);
		twoaway = current+next;
		current = next;
		next    = twoaway;
    }

	return EXIT_SUCCESS;
}

