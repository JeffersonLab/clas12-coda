
/* sis3801init.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef Linux_vme

#include "jvme.h"
#include "sis3801.h"
static int nsis;
unsigned int addr;
#define MASK    0x00000000   /* unmask all 32 channels (0-enable,1-disable) */

/* general settings */
static void
sis3801config(int id, int mode)
{
  sis3801control(id, DISABLE_EXT_NEXT);
  sis3801reset(id);
  sis3801clear(id);
  sis3801setinputmode(id,mode);
  sis3801enablenextlogic(id);
  sis3801control(id, ENABLE_EXT_DIS);
}

static int mode = 2;


int
main(int argc, char *argv[])
{
  int id, res;
  char myname[256];
  unsigned int addr, laddr;
  int slot = 0;


  /* Open the default VME windows */
  vmeOpenDefaultWindows();
  printf("\n");


  printf("SIS3801 Download() starts =========================\n");

vmeBusLock();

  mode = 2; /* Control Inputs mode = 2  */
  nsis = sis3801Init(0x10000000, 0x1000000, 2, mode);
  /*if(nsis>0) TDC_READ_CONF_FILE;*/

  for(id = 0; id < nsis; id++)
  {
    sis3801config(id, mode);
    sis3801control(id, DISABLE_EXT_NEXT);

    printf("    Status = 0x%08x\n",sis3801status(id));
  }

vmeBusUnlock();

  printf("SIS3801 Download() ends =========================\n\n");




vmeBusLock();
  for(id = 0; id < nsis; id++)
  {
    /*sis3801clear(id);*/
    sis3801config(id, mode);
    sis3801control(id, DISABLE_EXT_NEXT);
  }
vmeBusUnlock();


  for(id = 0; id < nsis; id++)
  {
vmeBusLock();
    sis3801control(id, DISABLE_EXT_NEXT);
vmeBusUnlock();
    printf("    Status = 0x%08x\n",sis3801status(id));
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
