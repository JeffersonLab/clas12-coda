/* petirocsynccal.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "petirocLib.h"
#include "petirocConfig.h"

static int npetiroc;

int main(int argc, char *argv[])
{
  PETIROC_Regs chip[2];
  printf("\npetirocsyncal started ..\n\n");fflush(stdout);

  npetiroc = petirocInit(0, PETIROC_MAX_NUM, PETIROC_INIT_REGSOCKET);

  printf("\npetirocinit: npetiroc=%d\n\n",npetiroc);fflush(stdout);
  if(npetiroc<=0) exit(0);

  for(int j=0; j<npetiroc; j++)
  {
    int slot = petirocSlot(j);
    petiroc_Reboot(slot, 1);
  }
   
  petirocEnd();

  exit(0);
}

