
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
  int slot = 0;

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
    printf("Usage: sspfirmware <bin file> [slot]\n");
    exit(0);
  }
  printf("\n");

  /* Open the default VME windows */
  vmeOpenDefaultWindows();
  printf("\n");

  sspInit(slot<<19, 1<<19, 20, SSP_INIT_NO_INIT | SSP_INIT_SKIP_FIRMWARE_CHECK);

  /* update firmware 
  sspFirmwareUpdateVerify(slot, myname);
  */

  exit(0);
}

#else

int
main()
{
  return(0);
}
#endif
