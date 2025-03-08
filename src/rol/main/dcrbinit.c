
/* dcrbinit.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dcrbLib.h"


#ifdef Linux_vme

#include "jvme.h"

int
main(int argc, char *argv[])
{
  int res;
  char myname[256], result;
  unsigned int addr, laddr;
  int slot = 0;

  /* Open the default VME windows */
  vmeOpenDefaultWindows();
  printf("\n");

  dcrbInit((3<<19), 0x80000, 20, 0);

  dcrbGStatus(1);

  exit(0);
}

#else

int
main()
{
  return(0);
}
#endif
