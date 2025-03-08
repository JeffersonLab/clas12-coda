/* petirocthresholdscan.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "petirocLib.h"
#include "petirocConfig.h"

static int npetiroc;

extern PETIROC_CONF *petirocConfPtr; 

#define THR_START   300
#define THR_NUM     40
#define THR_STEP    5

int scalers[32][THR_NUM][52];

int main(int argc, char *argv[])
{
  memset(scalers, 0, sizeof(scalers));

  PETIROC_Regs chip[2];
  printf("\npetirocthresholdscan started ..\n\n");fflush(stdout);

  npetiroc = petirocInit(0, PETIROC_MAX_NUM, PETIROC_INIT_REGSOCKET);

  printf("\npetirocinit: npetiroc=%d\n\n",npetiroc);fflush(stdout);
  if(npetiroc<=0) exit(0);

  petirocInitGlobals();
  petirocConfig("");
  printf("\npetiroc initialized\n\n");fflush(stdout);


  for(int nthr=0; nthr<THR_NUM; nthr++)
  {
    int thr = THR_START+nthr*THR_STEP;
    printf("thr=%d\n", thr); fflush(stdout);
    for(int ch=0; ch<52; ch++)
    {
      for(int j=0; j<npetiroc; j++)
      {
        int slot = petirocSlot(j);
        petirocConfPtr[slot].chip[0].SlowControl.vth_time = thr;
        petirocConfPtr[slot].chip[1].SlowControl.vth_time = thr;
        petirocConfPtr[slot].chip[0].SlowControl.mask_discri_time = 0xFFFFFFFF;
        petirocConfPtr[slot].chip[1].SlowControl.mask_discri_time = 0xFFFFFFFF;
        petirocConfPtr[slot].chip[0].SlowControl.mask_discri_charge = 0xFFFFFFFF;
        petirocConfPtr[slot].chip[1].SlowControl.mask_discri_charge = 0xFFFFFFFF;
        if(ch<32)
          petirocConfPtr[slot].chip[0].SlowControl.mask_discri_time^=(1<<ch);
        else
          petirocConfPtr[slot].chip[1].SlowControl.mask_discri_time^=(1<<(ch-32));

        petiroc_cfg_rst(slot);
        petiroc_slow_control(slot, petirocConfPtr[slot].chip);
      
        usleep(10000);
        petiroc_clear_scalers(slot);
      }
      usleep(100000);
      for(int j=0; j<npetiroc; j++)
      {
        int slot = petirocSlot(j);
        scalers[slot][nthr][ch] = petiroc_get_scaler(slot, ch);
      }
    }
    
    for(int j=0; j<npetiroc; j++)
    {
      int slot = petirocSlot(j);
      printf("slot=%d:", slot);
      for(int ch=0; ch<52; ch++)
      {
        printf(" %d", scalers[slot][nthr][ch]);
      }
      printf("\n");
    }
  }

  petirocEnd();

  exit(0);
}

