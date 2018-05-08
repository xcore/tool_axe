// RUN: xcc -target=XCORE-200-EXPLORER -fcmdline-buffer-bytes=512 %s -o %t1.xe
// RUN: %sim --args %t1.xe hello world
#include "string.h"

int main(int argc, char **argv) {
  if (argc != 3) {
    return 1;
  }

  if (strcmp(argv[1], "hello") != 0) {
    return 2;
  }

  if (strcmp(argv[2], "world") != 0) {
    return 3;
  }

  return 0;
}
