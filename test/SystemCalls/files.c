// RUN: xcc -target=XK-1A %s -o %t1.xe -DFILENAME=\"%t2\"
// RUN: axe %t1.xe
// RUN: xcc -target=XCORE-200-EXPLORER %s -o %t1.xe -DFILENAME=\"%t2\"
// RUN: axe %t1.xe

#include <syscall.h>
#include <string.h>

#ifndef FILENAME
#error "Must define FILENAME"
#endif

int main()
{
  char buf[4];
  int fd = _open(FILENAME, O_CREAT | O_WRONLY, 0644);
  if (fd < 0)
    return 1;
  if (_write(fd, "foo", 4) != 4)
    return 1;
  if (_close(fd) < 0)
    return 1;

  fd = _open(FILENAME, O_RDONLY, 0);
  if (fd < 0)
    return 1;
  if (_read(fd, buf, 4) != 4)
    return 1;
  if (memcmp(buf, "foo", 4) != 0)
    return 1;
  if (_close(fd) < 0)
    return 1;

  return 0;
}
