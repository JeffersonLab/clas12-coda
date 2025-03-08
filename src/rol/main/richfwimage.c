
/* richfirmware.c */
/*
richfirmware("ssp.bin",13)


cd $CLON_PARMS/firmwares
richfirmware fe_rich.bin

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sspLib.h"
#include "sspLib_rich.h"


#ifdef Linux_vme

#include "jvme.h"

int
main(int argc, char *argv[])
{
  int res, i;
  char myname[256];
  unsigned int addr, laddr;
  int slot = 0, nssp = 0, image = 0;

  printf("\n");
  if(argc==3)
  {
	  slot = atoi(argv[1]);
    image = atoi(argv[2]);
    printf("Upgrade board at slot=%d only\n",slot);
  }
  else
  {
    printf("Usage: richfwimage <slot> <image=0 or 1>\n");
    exit(0);
  }
  printf("\n");

  /* Open the default VME windows */
  printf("vmeOpenDefaultWindows\n");
  vmeOpenDefaultWindows();
  printf("\n");

  printf("sspInit\n");
  sspInit(slot<<19, 0, 1, SSP_INIT_NO_INIT | SSP_INIT_SKIP_FIRMWARE_CHECK);
  printf("sspRich_Init\n");
  sspRich_Init(slot);
  printf("sspRich_RebootSlot\n");
  sspRich_RebootSlot(slot, image);

  exit(0);
}

#else

int
main()
{
  return(0);
}
#endif
