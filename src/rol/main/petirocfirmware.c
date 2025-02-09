/* petirocfirmware.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "petirocLib.h"
#include "petirocConfig.h"

static int npetiroc;

int main(int argc, char *argv[])
{
  printf("\npetirocfirmware started ..\n\n");fflush(stdout);
  if(argc != 2)
  {
    printf("Usage: petirocfirmware <firmwareimage.bin>\n");
    exit(1);
  }

  npetiroc = petirocInit(0, PETIROC_MAX_NUM, PETIROC_INIT_REGSOCKET);

  printf("\npetirocinit: npetiroc=%d\n\n",npetiroc);fflush(stdout);
  if(npetiroc<=0)
    exit(0);

  if(petiroc_flash_GFirmwareUpdate(argv[1]) == OK)
    petiroc_flash_GFirmwareVerify(argv[1]);

  printf("\npetiroc firmware update complete\n\n");
  fflush(stdout);
  petirocEnd();

  exit(0);
}

