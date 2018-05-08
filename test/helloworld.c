// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: %sim %t1.xe > %t2.txt
// RUN: cmp %t2.txt %s.expect
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: %sim %t1.xe > %t2.txt
// RUN: cmp %t2.txt %s.expect

#include <stdio.h>

int main()
{
  printf("Hello world\n");
  return 0;
}
