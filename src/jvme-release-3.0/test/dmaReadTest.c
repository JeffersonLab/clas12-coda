/*
 * File:
 *    dmaReadTest.c
 *
 * Description:
 *    Test routine to perform DMA from "slave" module
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "jvme.h"
#include "dmaPList.h"

char *program_name;

void
usage()
{
  printf("Usage: \n\t");

  printf("%s: <size [bytes]> <address modifier: 0x29 / 0x39 / 0x09> <vme address>\n",
	 program_name);
}

#define BUFFER_SIZE 1024*20
#define NBUFFER     200

int32_t
main(int32_t argc, char *argv[]) {

  void *localp = NULL;
  uint32_t size = 0, amcode = 0, vme_addr = 0xa80000, val = 0;
  int32_t stat = 0, addrType = 0;
  DMA_MEM_ID vmeIN, vmeOUT;

  program_name = argv[0];
  if(argc != 4)
    {
      usage();
      exit(-1);
    }

  size  = strtoll(argv[1], NULL, 10);

  amcode = strtoll(argv[2], NULL, 16) & 0xffffffff;
  vme_addr = strtoll(argv[3], NULL, 16) & 0xffffffff;

  stat = vmeOpen();

  if(stat != OK)
    goto CLOSE;

  /* Setup Address and data modes for DMA transfers
   *
   *  vmeDmaConfig(addrType, dataType, sstMode);
   *
   *  addrType = 0 (A16)    1 (A24)    2 (A32)
   *  dataType = 0 (D16)    1 (D32)    2 (BLK32) 3 (MBLK) 4 (2eVME) 5 (2eSST)
   *  sstMode  = 0 (SST160) 1 (SST267) 2 (SST320)
   */
  if(amcode == 0x29)
    addrType = 0;
  else if(amcode == 0x39)
    addrType = 1;
  else if(amcode == 0x09)
    addrType = 2;
  else
    {
      usage();
      goto CLOSE;
    }

  vmeDmaConfig(addrType, 3, 2);

  /* INIT dmaPList */
  dmaPFreeAll();
  vmeIN  = dmaPCreate("vmeIN", BUFFER_SIZE, NBUFFER, 0);
  vmeOUT = dmaPCreate("vmeOUT", 0, 0, 0);

  dmaPStatsAll();

  dmaPReInitAll();

  printf("Press return to perform DMA read from 0x%x\n", vme_addr);
  getchar();


  // Perform read here
  /* Buffer node pointer */
  extern DMANODE *the_event;
  /* Pointer to current position in data buffer */
  extern unsigned int *dma_dabufp;

  /***********************************/
  /* USING MACROS WITH EVENT BUFFERS */

  /* Acquire buffer from vmeIN list, assign dma_dabufp to data[0] */
  GETEVENT(vmeIN, 0);
  stat = vmeDmaSend((unsigned long)dma_dabufp, vme_addr, size);

  if(stat != 0)
    printf("bad\n");

  stat = vmeDmaDone();
  printf("stat = %d\n", stat);

  if(stat > 0)
    dma_dabufp += stat>>2;

  /* Push this event buffer to vmeOUT list  */
  PUTEVENT(vmeOUT);


  /*************************************/
  /* USING ROUTINES WITH EVENT BUFFERS */

  int32_t len, ii;
  DMANODE *outEvent;

  /* Acquire buffer from vmeOUT list  */
  outEvent = dmaPGetItem(vmeOUT);

  if(outEvent != NULL) {
    len = outEvent->length;

    for(ii=0; ii<len; ii++) {
      if((ii%5) == 0) printf("\n    ");
      printf(" 0x%08x ",(uint32_t)bswap_32(outEvent->data[ii]));
    }
    printf("\n\n");

    /* Free this event buffer  */
    dmaPFreeItem(outEvent);
  }else{
    logMsg("Error: no Event in vmeOUT queue\n",0,0,0,0,0,0);
  }



 CLOSE:

  /* Free all allocated event buffers */
  dmaPFreeAll();

  stat = vmeClose();
  if (stat != OK)
    return -1;

  exit(0);
}

/*
  Local Variables:
  compile-command: "make -k dmaReadTest "
  End:
*/
