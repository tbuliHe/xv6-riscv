#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int num = getprocnum();
  printf("Number of processes: %d\n", num);
  exit(0);
}
