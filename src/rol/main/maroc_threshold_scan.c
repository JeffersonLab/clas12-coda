#ifndef Linux_armv7l

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <marocLib.h>
#include <marocConfig.h>

static int nmaroc;

void scaler_print(int p)
{
  unsigned int scalers[192];
  unsigned int pixels[192];
  int i, j, slot;

  for(int m=0; m<nmaroc; m++)
  {
    slot = marocSlot(m);
    if(p) printf("%s - SLOT=%d\n", __func__, slot);

    maroc_get_scalers(slot, scalers, 1);

    if(p)
    {
      for(i=0;i<192;i++)
      {
        int asic=i/64;
        int maroc_ch=i%64;
        int pixel = asic*64+maroc_ch_to_pixel[maroc_ch];
        pixels[pixel] = scalers[i];
      }

      for(i=0;i<8;i++)
      {
        for(j=0;j<24;j++)
        {
          int pmt = j/8;
          int col = j%8;
          int row = i;
          printf("%7u",pixels[64*pmt+8*row+col]);

          if(col==7 && pmt<2)
            printf(" | ");
        }
        printf("\n");
      }
      printf("\n");
      fflush(stdout);
    }
  }
}

#define THR_MIN   140
#define THR_MAX   250

int
main(int argc, char *argv[])
{
  int res;
  char myname[256];
  unsigned int addr, laddr;
  int slot = 0, chip, thr = THR_MIN;

  nmaroc = marocInit(0, 1);//MAROC_MAX_NUM);

  printf("\nmarocinit: nmaroc=%d\n\n",nmaroc);fflush(stdout);
  if(nmaroc<=0) exit(0);

  marocInitGlobals();
  marocConfig("");
  printf("\nmaroc initialized\n\n");fflush(stdout);

  while(1)
  {
    for(slot=0;slot<nmaroc;slot++)
    {
      for(chip=0;chip<3;chip++)
        maroc_SetMarocReg(slot, chip, MAROC_REG_DAC0, 0, thr);

      maroc_UpdateMarocRegs(slot);
    }
    scaler_print(0); // dummy access to clear after reconfig
    usleep(1000000);
    printf("scaler threshold = %d\n", thr);
    scaler_print(1);
    if(++thr>THR_MAX)
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

