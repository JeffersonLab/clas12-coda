
/* sspfirmware.c */
/*
sspfirmware("ssp.bin",13)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sspLib.h"


#ifdef Linux_vme

#include "jvme.h"

int
main(int argc, char *argv[])
{
  int res;
  char myname[256];
  unsigned int addr, laddr;
  int slot = 0, val = 0, fiber;

  printf("\n");
  if(argc==3)
  {
	  slot = atoi(argv[1]);
    val = atoi(argv[2]);
    printf("test board at slot=%d only\n",slot);
	}
  else
  {
    printf("Usage: ssptest <slot> <val>\n");
    exit(0);
  }
  printf("\n");

  /* Open the default VME windows */
  vmeOpenDefaultWindows();
  printf("\n");

  sspInit(slot<<19, 1<<19, 1, SSP_INIT_NO_INIT | SSP_INIT_SKIP_FIRMWARE_CHECK);

//  sspQSFPStatus(slot);

  if(val)
  {
    printf("Resetting QSFP...\n"); 
    for(fiber=0;fiber<8;fiber++)
      sspQSFPReset(slot, fiber, 1);

    usleep(1000000);

    for(fiber=0;fiber<8;fiber++)
      sspQSFPReset(slot, fiber, 0);

  }

  sspQSFPStatus(slot);
  exit(0);
}

#else

int
main()
{
  return(0);
}
#endif
