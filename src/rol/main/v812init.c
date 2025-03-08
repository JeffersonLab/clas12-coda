
/* v812init.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef Linux_vme

#include "jvme.h"
#include "v812.h"

int
main(int argc, char *argv[])
{
  int nv812, id;

  /* Open the default VME windows */
  vmeOpenDefaultWindows();

vmeBusLock();
  nv812 = v812Init();
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

