
/* vmeservermain.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef Linux_vme

#include "jvme.h"
#include "V1495VMERemote.h"
#include "libtcp.h" 
#include "vmeserver.h"

int
main(int argc, char *argv[])
{
  char myname[256];

  /* Open the default VME windows */
  vmeOpenDefaultWindows();

  if(argc==2)
  {
    strncpy(myname, argv[1], 255);
    printf("use argument >%s< as host name\n",myname);
  }
  else
  {
    
    strncpy(myname, getenv("HOSTNAME"), 255);
    printf("use env var HOST >%s< as host name\n",myname);
  }

  vmeServer(myname);
  while(1) sleep(1);
}

#else

int
main()
{
}
#endif

