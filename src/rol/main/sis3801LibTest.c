#if defined(VXWORKS) || defined(Linux_vme)

/*
 * File:
 *    sis3801LibTest.c
 *
 * Description:
 *    Program to test sis3801 library
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "jvme.h"
#include "sis3801.h"

void getFromHost(char** a,int b,int c,char** d){}
extern unsigned long addr;

void
myISR(int arg)
{
  int i, rval, dready = 0, timeout = 0;
  const int len = 256;
  unsigned int derta[len];
  static int intCalls = 0;

  printf(" try to lock\n");
  vmeBusLock();
  printf(" lock\n");
  memset(&derta, 0, sizeof(derta));

  while((dready == 0) && (timeout++ < 100))
    {
      dready = (sis3801status(addr) & FIFO_EMPTY) ? 0 : 1;
    }

  printf("%s: Call count = %d    Status = 0x%08x  dready = %d\n",
	 __func__, ++intCalls, sis3801status(addr), dready);

  if(dready == 0)
    {
      vmeBusUnlock();
      return;
    }

  derta[0] = len;
  rval = sis3801read(addr, &derta);

  printf(" return length (bytes) = %d\n",
	 rval);

  for(i = 0; i < (rval>>2); i++)
    {
      if((i%4) == 0)
	printf("\n%4d: ", i);
      printf(" 0x%08x ",
	     derta[i]);
    }
  printf("\n");
  vmeBusUnlock();

  return (0);
}

int
main(int argc, char *argv[])
{
  int nsis, iflag=0;
  int stat;

  if(vmeOpenDefaultWindows() != OK)
    goto CLOSE;



  nsis = sis3801Init(/*0x11100000*//*0x280800*/0x100000,0x80000,10,iflag);
  printf("nsis=%d\n",nsis);


  exit(0);








  if(sis3801CheckAddresses() != OK)
    goto CLOSE;
  printf("\nsis3801 library test\n");
  printf("--------------------------------------------------------------------------------\n");

  vmeCheckMutexHealth(2);

  vmeBusLock();
  l2_init();
  sis3801reset(addr);
  scalIntConnect(myISR, 0);
  sis3801control(addr,
		 ENABLE_25MHZ|
		 ENABLE_PRESCALER|
		 ENABLE_10MHZ_TO_LNE_PRESCALER|
		 ENABLE_TEST
		 );
  sis3801setlneprescaler(addr, 0xffffff);
  printf("    Status = 0x%08x\n",
	 sis3801status(addr));
  sis3801clear(addr);
  sis3801enablenextlogic(addr);
  sis3801nextclock(addr); // Start the counters.. will generate interrupt if enabled

  scalIntEnable(0x1);

  sis3801nextclock(addr);
  sis3801testclock(addr);


  printf("Press <Enter> to start interrupts\n");
  getchar();

  vmeBusUnlock();

  printf("Press <Enter> to stop interrupts\n");
  getchar();
  vmeBusLock();
  printf("    Status = 0x%08x\n",
	 sis3801status(addr));
  sis3801control(addr,
		 DISABLE_25MHZ|
		 DISABLE_PRESCALER|
		 DISABLE_10MHZ_TO_LNE_PRESCALER|
		 DISABLE_TEST
		 );
  scalIntDisable();
  vmeBusUnlock();

CLOSE:

  vmeCloseDefaultWindows();

  exit(0);
}


/*
  Local Variables:
  compile-command: "make -k -B s3801LibTest"
  End:
 */

#else /* dummy version*/

void
sis3801LibTest_dummy()
{
  return;
}

#endif
