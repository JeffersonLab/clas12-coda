/* petirocsynccal.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "petirocLib.h"
#include "petirocConfig.h"
#include "TIpcieUSLib.h"

static int npetiroc;

int main(int argc, char *argv[])
{
  PETIROC_Regs chip[2];
  printf("\npetirocsyncal started ..\n\n");fflush(stdout);

  if(tipusOpen()!=OK)
    goto CLOSE;

  tipusInit(0,TIPUS_INIT_SKIP_FIRMWARE_CHECK);
  tipusSetBusySource(0,1); /* remove all busy conditions */
  tipusIntDisable();
  tipusSetPrescale(0);
  tipusDisableTSInput(TIPUS_TSINPUT_ALL);
  tipusStatus(1);

  npetiroc = petirocInit(0, 1/*PETIROC_MAX_NUM*/, PETIROC_INIT_REGSOCKET);

  printf("\npetirocinit: npetiroc=%d\n\n",npetiroc);fflush(stdout);
  if(npetiroc<=0) exit(0);

  petirocInitGlobals();
  petirocConfig("");
  printf("\npetiroc initialized\n\n");fflush(stdout);

  petiroc_status_all(); 
 
  tipusSoftTrig(1,100,100,0);
  for(int j=0; j<100; j++)
  { 
    tipusSyncReset(1);
    usleep(50);
  }

  for(int j=0; j<npetiroc; j++)
  {
    int slot = petirocSlot(j);
//      petiroc_set_idelay(slot, d, d+5, d, d+5);
    petiroc_get_idelay_status(slot);
  }
   
  printf("\n");
  petiroc_gstatus(); 
  
  petiroc_status_all(); 

  petirocEnd();

CLOSE:
  tipusClose();

  exit(0);
}

