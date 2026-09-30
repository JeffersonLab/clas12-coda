/*
 * File:
 *    SlaveWindow.c
 *
 * Description:
 *    Test Slave Windows
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <byteswap.h>
#include "jvme.h"
#include "jvmeWindows.h"

int
main(int argc, char *argv[])
{
  uint32_t vmeAddr = 0x18000000;
  uint32_t *localPtr = NULL;
  uint16_t *localAddr;
  int stat = 0;

  stat = jvmeOpenSlaveWindow(JVME_A32, (void**)&localPtr);
  printf(" jvmeOpenSlaveWindow returned  %d\n",
	 stat);

  if(stat == ERROR)
    exit(0);

  printf(" localPtr = 0x%lx\n",
	 localPtr);

  localAddr = (uint16_t *)&localPtr[0];

  printf("localPtr[0x%x] = 0x%lx\n",
	 localAddr, bswap_16(localAddr[0]));

  /* localAddr[0] = 0x1234; */

  stat = jvmeCloseSlaveWindow(JVME_A32);
  printf(" jvmeCloseSlaveWindow returned %d\n",
	 stat);

  exit(0);
}
