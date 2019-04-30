
/* moinit.c */

/*USAGE: must be on the VME controller, type: 'moinit' */

/*
The module must be initialized with it's A24 address (addr) with: 
   moInit(addr, flag);

where flag is a bit mask:
   0 - No module initialization (just library initialization)
   1 - Ignore module firmware check.

As requested, there are four simplified routines for normal operation:

  1st stage prescale:
  int  moSetInitialPrescale(uint32_t prescale);
  int  moGetInitialPrescale(uint32_t *prescale);

  2nd stage prescale:
  int  moSetPrescale(uint32_t channel, uint32_t prescale);
  int  moGetPrescale(uint32_t channel, uint32_t *prescale);

where:
   channel = [0,9] (boardRev 0x1), [0,5] (boardRev 0x2)
   prescale = [1,32]

A status printout can be obtained from:

  int  moConfigPrint();
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#ifdef Linux_vme

#include "moLib.h"
#include "jvme.h"

int
main(int argc, char *argv[])
{
  int ret, flag;
  unsigned int addr = 0xA00000; /*slot 20*/

  /* Open the default VME windows */
  vmeOpenDefaultWindows();

  /*
vmeBusLock();
  tiInit((21<<19),2,0);
  tiStatus(1);
vmeBusUnlock();
  */

  printf("argc=%d\n",argc);
  if(argc==2)
  {
    printf("argv=>%s<\n",argv[1]);
    addr = strtol(argv[1], (char **)NULL, 16);
  }

  printf("use addr=0x%04x\n",addr);


vmeBusLock();

  flag = 0;
  ret = moInit(addr, flag);
  moConfigPrint();

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
