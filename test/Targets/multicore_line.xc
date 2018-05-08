// RUN: xcc %s.xn %s -o %t1.xe
// RUN: %sim %t1.xe > %t2.txt
// RUN: cmp %t2.txt %s.expect

#include <print.h>
#include <platform.h>

int main()
{
  par {
    on tile[0]: printstr("hello\n");
    on tile[1]: printstr("hello\n");
    on tile[2]: printstr("hello\n");
    on tile[3]: printstr("hello\n");
    on tile[4]: printstr("hello\n");
    on tile[5]: printstr("hello\n");
    on tile[6]: printstr("hello\n");
    on tile[7]: printstr("hello\n");
  }
  return 0;
}
