// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: %sim %t1.xe
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe
// RUN: %sim %t1.xe

#include <stdio.h>
int main(void) {
  char* filename = "temp_axe_io_test.txt";
  char string[] = "abcdefghijklmnopqrstuvwxyz"; // "\0"
  FILE *fp = fopen(filename, "w+");
  if (!fp) return 1;
  if (fwrite(string, 1, sizeof(string),fp) != sizeof(string)) return 1;
  if (fseek(fp, -17, SEEK_CUR) != 0) return 1;
  if (fgetc(fp) != 'k') return 1;
  if (fseek(fp, 0, SEEK_SET) != 0) return 1;
  if (fgetc(fp) != 'a') return 1;
  if (fseek(fp, -2, SEEK_END) != 0) return 1;
  if (fgetc(fp) != 'z') return 1;
  if (fclose(fp) != 0) return 1;
  if (remove(filename) != 0) return 1;
  return 0;
}
