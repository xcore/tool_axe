// RUN: xcc %s.xn %s -o %t1.xe
// RUN: axe %t1.xe > %t2.txt
// RUN: cmp %t2.txt %s.expect

#include <print.h>
#include <platform.h>

int main()
{
  par {
    on tile[0]: printstr("hello\n");
  }
  return 0;
}
