
/* fadc250init.c */

/*USAGE: must be on the VME controller, type: 'fadc250init' */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#ifdef Linux_vme

#include "jvme.h"
#include "usrvme.h"

#include "codautil.h"
#include "tiLib.h"
#include "fadcLib.h"
#include "fadc250Config.h"

static char hostname[128];

int
main(int argc, char *argv[])
{
  int id, slot, status;
  int nadc;
  char *s, str[1024];

  /* get hostname */
  get_hostname(hostname,127);
  s = hostname;
  hostname[strlen(hostname)] = 0;
  while(*s)
  {
    if(*s == '.')
    {
      *s = 0x00;
      break;
    }
    // *s = tolower((unsigned char) *s);
    s++;
  }
  printf("Our hostname is %s\n",hostname);

  /* Open the default VME windows */
  vmeOpenDefaultWindows();

vmeBusLock();
  tiInit(0/*(21<<19)*/,2,0);
  tiStatus(1);
vmeBusUnlock();




vmeBusLock();

  nadc = faInit(0x180000,0x80000,20,0);
  nadc = faGetNfadc();
  printf("\nFOUND %d FADC250's\n\n",nadc);
  sprintf(str,"/usr/clas12/release/2.0.0/parms/fadc250/%s.cnf",hostname);
  if(nadc)
  {
    fadc250Config(str);
  }

  for(id=0; id<nadc; id++)
  {
    slot = faSlot(id);
    faSetDAC(slot,3301,0/*0xFFFF*/);
  }

  for(id=0; id<nadc; id++)
  {
    slot = faSlot(id);
    printf("slot=%d =========================================\n",slot);
    faStatus(slot,0);
    faPrintDAC(slot);
  }

  fadc250UploadAllPrint();

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
