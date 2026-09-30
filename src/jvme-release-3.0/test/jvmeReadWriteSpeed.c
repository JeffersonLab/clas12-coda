/*
 * File:
 *    jvmeReadWriteSpeed.c
 *
 * Description:
 *    Perform a number of consecutive reads and writes on the VME bus
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <byteswap.h>
#include "jvme.h"
/* #include "jvmeWindows.h" */

int
main(int argc, char *argv[])
{
  uint32_t vmeAddr = (0xea00);
  volatile uint16_t *localPtr = NULL, val = 0;
  uint64_t laddr;

  int stat = 0;

  stat = vmeOpenDefaultWindows();
  printf(" jvmeOpenMasterWindow returned  %d\n",
	 stat);

  stat = vmeBusToLocalAdrs(0x29,(char *)(unsigned long)vmeAddr,(char **)&laddr);
  if (stat != 0)
    {
      printf("%s: ERROR: Error in vmeBusToLocalAdrs res=%d \n",__FUNCTION__,stat);
      return ERROR;
    }

  localPtr = (uint16_t *)laddr;
  val = vmeRead16(&localPtr[0]);
  printf(" val = 0x%04x\n", val);

  int i, n = 100;
  for(i = 0; i < n; i++)
    {
      val = vmeRead16(&localPtr[0]);
    }

  val = (1 << 5);
  for(i = 0; i < n; i++)
    {
      vmeWrite16(&localPtr[0], val);
    }

  stat = vmeCloseDefaultWindows();
  printf(" jvmeCloseMasterWindow returned %d\n",
	 stat);

  exit(0);
}

/*
  Local Variables:
  compile-command: "make -k -B jvmeReadWriteSpeed"
  End:
 */
