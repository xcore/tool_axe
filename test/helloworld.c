// RUN: xcc -target=XC-5 %s -o %t1.xe
// RUN: axe %t1.xe > %t2.txt
// RUN: cmp %t2.txt %s.expect

#include <stdio.h>

int main()
{
  printf("Hello world\n");
  return 0;
}
