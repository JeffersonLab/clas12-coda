
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
  int slot = 0, nssp = 0;

  printf("\n");
  if(argc==2||argc==3)
  {
    strncpy(myname, argv[1], 255);
    printf("Use argument >%s< as bin file name\n",myname);
    if(argc==3)
	{
	  slot = atoi(argv[2]);
      printf("Upgrade board at slot=%d only\n",slot);
	}
	else
	{
      slot = 0;
      printf("Upgrade all boards in crate\n");
	}
  }
  else
  {
    printf("Usage: richfirmware <bin file> [slot]\n");
    exit(0);
  }
  printf("\n");

  /* Open the default VME windows */
  vmeOpenDefaultWindows();
  printf("\n");

  if(slot)
  {
    sspInit(slot<<19, 0, 1, SSP_INIT_NO_INIT | SSP_INIT_SKIP_FIRMWARE_CHECK);
    sspRich_Init(slot);
    sspRich_FirmwareVerifyAll(slot, myname);
  }
  else
  {
    sspInit(0, 0, 0, SSP_INIT_NO_INIT | SSP_INIT_SKIP_FIRMWARE_CHECK);
    nssp = sspGetNssp();
    for(i=0; i<nssp; i++)
    {
      slot = sspSlot(i);
      if(sspGetFirmwareType(slot) == SSP_CFG_SSPTYPE_HALLBRICH)
      {
        sspRich_Init(slot);
        sspRich_FirmwareVerifyAll(slot, myname);
      }
    }
  }

  exit(0);
}

#else

int
main()
{
  return(0);
}
#endif
