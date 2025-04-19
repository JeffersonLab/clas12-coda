/* petiroc_setclk.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "petirocLib.h"
#include "petirocConfig.h"

static int npetiroc;

int main(int argc, char *argv[])
{
  int sel = 0;
  printf("\npetiroc_setclk started ..\n\n");fflush(stdout);
  if(argc!=2)
  {
    printf("Usage: petiroc_setclk <refclk>\n");
    printf("  refclk=0: internal, refclk=1: external\n"); 
    return -1;
  }

  sel = atoi(argv[1]);

  npetiroc = petirocInit(0, PETIROC_MAX_NUM, PETIROC_INIT_REGSOCKET);

  printf("\npetiroc_setclk: npetiroc=%d\n\n",npetiroc);fflush(stdout);
  if(npetiroc<=0) exit(0);

  for(int i=0; i<npetiroc; i++)
  {
    int slot = petirocSlot(i);
    petiroc_set_clk(slot, sel);
  }

  petirocEnd();

  exit(0);
}

