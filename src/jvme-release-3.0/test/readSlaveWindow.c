/*
 * File:
 *    readSlaveWindow.c
 *
 * Description:
 *    read from the Slave window (treating it like regular VME access)
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "jvme.h"

int
main(int argc, char *argv[])
{
  unsigned long laddr;
  unsigned int taddr = 0x09000000;
  int stat;
  int inputchar=10;
  int res;
  int rdata;

  vmeOpenDefaultWindows();

  res = vmeBusToLocalAdrs(0x09,(char *)(unsigned long)taddr,(char **)&laddr);
  if (res != 0)
    {
      printf("%s: ERROR in vmeBusToLocalAdrs(0x%x,0x%x,&laddr) \n",
	     __FUNCTION__,0x09,laddr);
      return(ERROR);
    }

  struct vmecpumem {
    unsigned int id;
  };
  struct vmecpumem *vmecpu;

  vmecpu = (struct vmecpumem *)laddr;
  res = vmeMemProbe((char *) &(vmecpu->id), 4, (char *)&rdata);

  printf("res = %d,  rdata = 0x%x\n", res, LSWAP(rdata));
  vmeCloseDefaultWindows();
  exit(0);

 CLOSE:

  vmeCloseDefaultWindows();

  exit(0);
}
