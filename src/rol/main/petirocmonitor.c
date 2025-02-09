/* petirocmonitor.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "petirocLib.h"
#include "petirocConfig.h"

static int npetiroc;

int main(int argc, char *argv[])
{
  printf("\npetirocmonitor started ..\n\n");fflush(stdout);

  npetiroc = petirocInit(0, PETIROC_MAX_NUM, PETIROC_INIT_REGSOCKET);

  printf("\npetirocmonitor: npetiroc=%d\n\n",npetiroc);fflush(stdout);
  if(npetiroc<=0) exit(0);

  for(int i=0; i<npetiroc; i++)
  {
    int slot = petirocSlot(i);
    petiroc_printmonitor(slot);
  }

  petirocInitGlobals();
  petirocConfig("");
  petiroc_gstatus();
  if(argc==3)
  {
    for(int i=0; i<npetiroc; i++)
    {
      int slot = petirocSlot(i);
      probe(slot, atoi(argv[1]), atoi(argv[2]), 0, 0);
    }
  }
  printf("\npetiroc initialized\n\n");fflush(stdout);

  petirocEnd();

  exit(0);
}

