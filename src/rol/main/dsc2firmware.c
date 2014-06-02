
/* dsc2firmware.c */

/* UNIX version usage:

  cd $CLON_PARMS/firmwares
  dsc2firmware vmeDSC_firmware_v1_c.bin

VXWORKS:

  dsc2Init(0x100000,0x80000,16,0xc0000)
  dsc2UpdateFirmwareAll("/usr/local/clas12/release/0.1/parms/firmwares/vmeDSC_firmware_v1_c.bin")

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dsc2Lib.h"



#ifdef Linux_vme

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
    printf("Usage: dsc2firmware <bin file> [slot]\n");
    exit(0);
  }
  printf("\n");

  /* Open the default VME windows */
  vmeOpenDefaultWindows();
  printf("\n");

  /* update firmware */
  
  dsc2Init(0x100000,0x80000,20,0xc0000);
  dsc2UpdateFirmwareAll(myname);

  exit(0);
}

#else

int
main()
{
  return(0);
}

#endif
