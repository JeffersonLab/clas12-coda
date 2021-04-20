#if defined(VXWORKS) || defined(Linux_vme)

/*
 * File:
 *    s3801LibTest.c
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

int Nsis = 0;

void
myISR(int arg)
{
  int i, rval, dready = 0, timeout = 0, isca ;
  const int len = 256;
  unsigned int derta[len];
  static int intCalls = 0;

  vmeBusLock();

  intCalls++;
  for(isca = 0; isca < Nsis; isca++)
    {
      while((dready == 0) && (timeout++ < 100))
	{
	  dready = (sis3801status(isca) & FIFO_EMPTY) ? 0 : 1;
	}

      printf("%s(id = %d): Call count = %d    Status = 0x%08x  dready = %d\n",
	     __func__, isca, intCalls, sis3801status(isca), dready);

      if(dready == 0)
	{
	  vmeBusUnlock();
	  return;
	}

      derta[0] = len;
      rval = sis3801read(isca, (unsigned int *)&derta);

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
    }

  vmeBusUnlock();

}

int
main(int argc, char *argv[])
{
  int mode = 0, isca = 0;

  if(vmeOpenDefaultWindows() != OK)
    goto CLOSE;

  if(sis3801CheckAddresses() != OK)
    goto CLOSE;
  printf("\nsis3801 library test\n");
  printf("--------------------------------------------------------------------------------\n");

  vmeCheckMutexHealth(2);

  vmeBusLock();

  mode = 2; /* Control Inputs mode = 2  */
  Nsis = sis3801Init(0x200000, 0x100000, 2, mode);

  for(isca = 0; isca < Nsis; isca++)
    {
      sis3801reset(isca);
      sis3801clear(isca);
      sis3801enablenextlogic(isca);
      printf("    Status = 0x%08x\n",
	     sis3801status(isca));
    }

  /* Set up the 0th scaler as the interrupt source */
  /* 2nd arg: vector = 0 := use default */
  scalIntInit(0, 0);

  /* Connect service routine */
  scalIntConnect(myISR, 0);

  printf("Press <Enter> to start interrupts\n");
  getchar();

  /* Enable interrupts */
  scalIntEnable(0x1);

  /* Enable external next (control input #1) */
  for(isca = 0; isca < Nsis; isca++)
    {
      sis3801control(isca, ENABLE_EXT_NEXT);
    }

  vmeBusUnlock();

  printf("Press <Enter> to stop interrupts\n");
  getchar();
  vmeBusLock();

  for(isca = 0; isca < Nsis; isca++)
    {
      printf("    Status = 0x%08x\n",
	     sis3801status(isca));

      sis3801control(isca, DISABLE_EXT_NEXT);
    }
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

int
main()
{
}

#endif
