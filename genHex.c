// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv)
{
  FILE *in, *out;
  int c;
  int needComma;
  if (argc != 3) {
    fprintf(stderr, "Usage: genHex input output");
    return 1;
  }
  in = fopen(argv[1], "r");
  if (!in) {
    fprintf(stderr, "failed to open %s: %s", argv[1], strerror(errno));
    return 1;
  }
  out = fopen(argv[2], "w");
  if (!out) {
    fprintf(stderr, "failed to open %s: %s", argv[2], strerror(errno));
    return 1;
  }
  fprintf(out, "{ ");
  needComma = 0;
  while ((c = getc(in)) != EOF) {
    if (needComma) {
      fprintf(out, ", ");
    }
    fprintf(out, "0x%02x", (unsigned)(unsigned char)c);
    needComma = 1;
  }
  fprintf(out, " }");
  return 0;
}
