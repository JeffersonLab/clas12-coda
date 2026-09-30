/*
 * File:
 *    jvmeTest.c
 *
 * Description:
 *    Test JLab Vme Driver
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <byteswap.h>
/* #include "jvme.h" */
#include "jvmeWindows.h"

int
main(int argc, char *argv[])
{
  int stat = 0;

  stat = jvmeOpenDefaultWindows();
  printf("  stat = %d\n", stat);

  uint32_t tAddr = (5<<19), rAddr = 0;
  unsigned long laddr = 0;
  uint16_t am = 0;
  uint32_t *ptr;

  stat = jvmeBusToLocalAdrs(0x39,(char *)(unsigned long)tAddr,(char **)&laddr);

  if (stat != 0)
    {
      printf("%s: ERROR: Error in vmeBusToLocalAdrs res=%d \n",__FUNCTION__,stat);
    }
  else
    {
      printf("  tAddr = 0x%08x   laddr = 0x%lx\n",
	     tAddr, laddr);
    }

  ptr = (uint32_t *)laddr;
  printf("ptr[0] = 0x%08x\n",
	 ptr[0]);

  ptr[0x64>>2] = bswap_32(0x33224411);

  printf("  jvmeBusRead32(0x39, 0x%x) = 0x%08x\n",
	 tAddr + (0x64),
	 jvmeBusRead32(0x39, tAddr+0x64));

  jvmeBusWrite32(0x39, tAddr+0x64, 0x12345678);

  printf("  jvmeBusRead32(0x39, 0x%x) = 0x%08x\n",
	 tAddr + (0x64),
	 jvmeBusRead32(0x39, tAddr+0x64));

  stat = jvmeCloseMasterWindows(0xf);
  printf("  stat = %d\n", stat);

  exit(0);
}
