/*
 * RUN: xcc -target=XK-1A %s -o %t1.xe
 * RUN: %sim %t1.xe
 * RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
 * RUN: %sim %t1.xe
 */

int main() {
  return 0;
}
