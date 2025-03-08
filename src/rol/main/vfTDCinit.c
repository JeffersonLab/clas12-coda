
/* vfTDCinit.c */

/*USAGE: must be on the VME controller, type: 'vfTDCinit' */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef Linux_vme

#include "jvme.h"
#include "tiLib.h"
#include "vfTDCLib.h"


int
main(int argc, char *argv[])
{
  int nadc;

  /* Open the default VME windows */
  vmeOpenDefaultWindows();

vmeBusLock();
  tiInit(0/*(21<<19)*/,2,0);
  tiStatus(1);
vmeBusUnlock();


//vfTDCA32Base=0x09000000;
vmeBusLock();
 vfTDCInit(3<<19, 1<<19, 20, 
            VFTDC_INIT_VXS_SYNCRESET |
            VFTDC_INIT_VXS_TRIG      |
            VFTDC_INIT_VXS_CLKSRC);
vmeBusUnlock();


  //nadc = faInit(0x180000,0x80000,20,0);
  //if(nadc) fadc250Config("");

  exit(0);
}

#else

int
main()
{
  return(0);
}

#endif
