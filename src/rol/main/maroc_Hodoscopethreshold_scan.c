#ifndef Linux_armv7l

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "marocLib.h"
#include "marocConfig.h"

static int nmaroc;

void scaler_print(int thr, int slot)
{
  unsigned int scalers[192];
  unsigned int pixels[192];
  int i, j;

    //if(thr) printf("sl = %d %s - SLOT=%d\n", sl,  __func__, slot);

    maroc_get_scalers(slot, scalers, 1);

    printf("%d", thr);
    if(thr)
    {
      for(i=0;i<192;i++)
      {
	if( i >= 128 ){
	  printf(" %d", scalers[i]);
	}
      }
      printf("\n");

      fflush(stdout);
    }
  
}

#define THR_MIN   140
#define THR_MAX   1023

int
main(int argc, char *argv[])
{
  int res;
  char myname[256];
  unsigned int addr, laddr;
  int slot = 0, chip, thr = THR_MIN;

  if( argc == 2 ){
    slot = atoi(argv[1]);
  }

  nmaroc = marocInit(slot, 1);//MAROC_MAX_NUM); 2nd argument is the maximum number of slots (I think, Rafo)
  printf("nmaroc = %d \n", nmaroc);

  printf("\nmarocinit: nmaroc=%d\n\n",nmaroc);fflush(stdout);
  if(nmaroc<=0) exit(0);

  marocInitGlobals();
  marocConfig("");
  printf("\nmaroc initialized\n\n");fflush(stdout);

  while(1)
    {
      for(chip=0;chip<3;chip++){
        maroc_SetMarocReg(slot, chip, MAROC_REG_DAC0, 0, thr);
      }
      
      maroc_UpdateMarocRegs(slot);
      
      scaler_print(0, slot); // dummy access to clear after reconfig
      usleep(80000000);
      //printf("scaler threshold = %d\n", thr);
      scaler_print(thr, slot);
      
      thr = thr + 8;

      if(thr >THR_MAX)
	break;
    }

  exit(0);
}

#else

int main()
{
  return 0;
}

#endif

