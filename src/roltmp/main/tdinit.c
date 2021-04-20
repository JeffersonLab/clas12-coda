
/* dsc2init.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tdLib.h"



#ifdef Linux_vme

#include "jvme.h"

int
main(int argc, char *argv[])
{
  int res;
  char myname[256];
  unsigned int addr, laddr;
  int iflag, ntd, slot = 0;

  /*
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
    printf("Usage: dsc2init <bin file> [slot]\n");
    exit(0);
  }
  printf("\n");
  */

  /* Open the default VME windows */
  vmeOpenDefaultWindows();
  printf("\n");

  ntd = 0;
  iflag = 1;
  tdInit((3<<19),0x80000,20,iflag);
  ntd = tdGetNtds(); /* actual number of TD boards found  */

  tdGStatus(0);

  exit(0);
}

#else

int
main()
{
  return(0);
}

#endif
