
/* v851init.c */

/*USAGE: must be on the VME controller, type: 'v851init' */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef Linux_vme

#include "jvme.h"
#include "v851.h"


int
main(int argc, char *argv[])
{
  int ret;
  unsigned int addr = 0xc000;
  unsigned int addr1 = 0;

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

 printf("For the first board, use addr=0x%04x\n",addr);


vmeBusLock();

  printf("Programming board 0 at address 0x%04x\n",addr);
  ret = v851Init(addr,0);
  v851_start(100000,0);
  
  addr1 = addr + 0x1000;
  printf("Programming board 1 at address 0x%04x\n",addr1);
  ret = v851Init(addr1,1);
  v851_start(1000000,1);
  
vmeBusUnlock();


  exit(0);
}

#else

int
main()
{
}

#endif
