#ifndef THREADS
#define THREADS 1
#endif

extern int dhry();

int main()
{
#if THREADS == 1
  dhry();
#else
  par (int i = 0; i < THREADS; i++)
    dhry();
#endif
  return 0;
}
