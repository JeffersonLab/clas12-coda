
/* faV3init.c */

/*USAGE: must be on the VME controller, type: 'faV3init' */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef Linux_vme

#include "jvme.h"
#include "tiLib.h"
#include "faV3Lib.h"
#include "faV3Config.h"


int
main(int argc, char *argv[])
{
  int status;
  int nadc;

  /* Open the default VME windows */
  vmeOpenDefaultWindows();

vmeBusLock();
  tiInit(0/*(21<<19)*/,2,0);
  tiStatus(1);
vmeBusUnlock();

vmeBusLock();
  nadc = faV3Init(0x180000,0x80000,18,0);
  if(nadc)
  {
    faV3Config("");
  }
vmeBusUnlock();

  exit(0);
}

#else

int
main()
{
  return(0);
}

#endif
