// RUN: xcc -O2 -target=XK-1A %s -o %t1.xe
// RUN: not axe %t1.xe --max-cycles 10000
// RUN: xcc -O2 -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: not axe %t1.xe --max-cycles 10000

int main() {
  while (1) {}
  return 0;
}
