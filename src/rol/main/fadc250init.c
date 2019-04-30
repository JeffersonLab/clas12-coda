
/* fadc250init.c */

/*USAGE: must be on the VME controller, type: 'fadc250init' */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef Linux_vme

#include "jvme.h"
#include "tiLib.h"
#include "fadcLib.h"
#include "fadc250Config.h"


int
main(int argc, char *argv[])
{
  int nadc;

  /* Open the default VME windows */
  vmeOpenDefaultWindows();

vmeBusLock();
  tiInit((21<<19),2,0);
  tiStatus(1);
vmeBusUnlock();

  nadc = faInit(0x180000,0x80000,20,0);
  if(nadc) fadc250Config("");

  exit(0);
}

#else

int
main()
{
  return(0);
}

#endif
