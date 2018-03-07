// RUN: xcc -target=XS1-L2A-QF124 %s -o %t1.xe
// RUN: %sim %t1.xe > %t2.txt
// RUN: cmp %t2.txt %s.expect
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: %sim %t1.xe > %t2.txt
// RUN: cmp %t2.txt %s.expect

#include <print.h>
#include <platform.h>

int main()
{
  par {
    on stdcore[0]: printstr("hello\n");
    on stdcore[1]: printstr("hello\n");
  }
  return 0;
}
