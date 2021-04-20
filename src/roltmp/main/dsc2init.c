
/* dsc2init.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef Linux_vme

#include "jvme.h"
#include "dsc2Lib.h"
#include "dsc2Config.h"

int
main(int argc, char *argv[])
{
  int res;
  char myname[256];
  unsigned int addr, laddr;
  int slot = 0;

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

  
  dsc2Init(0x100000,0x80000,20,0/*1<<19*/);
  dsc2Config ("");

  exit(0);
}

#else

int
main()
{
  return(0);
}

#endif
