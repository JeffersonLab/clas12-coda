
#include <stdio.h>
#include <unistd.h>

int
main()
{
  int i;

  printf("start\n");fflush(stdout);

  for(i=0; i<10000; i++) usleep(10);

  printf("end\n");fflush(stdout);
}
