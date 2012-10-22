// RUN: xcc -target=XK-1A %s -o %t1.xe
// RUN: not axe %t1.xe > %t2.txt

#include <xs1.h>

unsigned alloc_lock()
{
  unsigned lock;
  asm("getr %0, 5" : "=r"(lock));
  return lock;
}

void acquire_lock(unsigned lock)
{
  asm("in %0, res[%0]" : : "r"(lock));
}

int x;

int main()
{
  chan c;
  unsigned lock = alloc_lock();
  acquire_lock(lock);
  par {
    c :> int;
    acquire_lock(lock);
    x = 1;
  }
  return 0;
}
