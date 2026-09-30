/*
 * File:
 *    jvmeWindowTest.c
 *
 * Description:
 *    Test VME Window open/close
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "jvme.h"
#include "jvmeWindows.h"

int
main(int argc, char *argv[])
{
  uint32_t vmeAddr = 0x280000, val;
  uint32_t *localPtr = NULL;
  uint32_t *localAddr;
  int stat = 0;
  uint32_t ambits = 0;

  ambits = 1<<JVME_A24;

  stat = jvmeOpenMasterWindows(ambits);
  printf(" jvmeOpenMasterWindow returned  %d\n",
	 stat);

  stat = jvmeBusToLocalAdrs(0x39, (char *) (unsigned long)vmeAddr, (char **) &localAddr);

  if(stat == OK)
    {
      val = vmeRead32(localAddr);
      printf("Read VME address 0x%x = 0x%08x\n",
	     vmeAddr, val);
    }

  stat = jvmeCloseMasterWindows(ambits);
  printf(" jvmeCloseMasterWindow returned %d\n",
	 stat);

  exit(0);
}

/*
  Local Variables:
  compile-command: "make -k -B jvmeWindowTest"
  End:
 */
