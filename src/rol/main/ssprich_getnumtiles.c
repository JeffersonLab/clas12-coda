#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sspLib.h"


#ifdef Linux_vme

#include "jvme.h"

int
main(int argc, char *argv[])
{
  int res, nssp, nfibers, fibers, i, j;
  char myname[256];
  unsigned int addr, laddr;
  int slot = 0, val = 0, fiber;

  /* Open the default VME windows */
  vmeOpenDefaultWindows();
  printf("\n");

  sspInit(0, 0, 1, SSP_INIT_SKIP_FIRMWARE_CHECK | SSP_INIT_NO_INIT);
  nssp = sspGetNssp();

  nfibers = 0;
  for(i=0;i<nssp;i++){
    slot = sspSlot(i);
	  sspRich_ScanFibers_NoInit(slot);
    sspRich_GetConnectedFibers(slot, &fibers);
    printf("SLOT[%d] sspRich_GetConnectedFibers  fibers = 0x%08X\n", slot, fibers);
    for(j=0;j<32;j++)
      if(fibers & (1<<j)) nfibers++;
  }

  printf("Total Tiles connected %d \n",nfibers);
  if(nfibers==0){
    printf("No fibers connected. Exit\n");
    return -1;
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
