/*
 * File:
 *    dmaPListSimple.c
 *
 * Description:
 *    Example use of dmaPList for managing a single DMA Buffer
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "jvme.h"
#include "dmaPList.h"

#define BUFFER_SIZE 1024*600
#define NBUFFER     1
#define MAX_DATA 32

int
main(int argc, char *argv[]) {

    int status;

    printf("\ndmaPList Simple Test\n");
    printf("----------------------------\n");

    DMA_MEM_ID vmeIN = dmaPCreate("vmeIN", BUFFER_SIZE, NBUFFER, 0);
    if(vmeIN == NULL)
      {
	printf("vmeIN = NULL \n");
	dmaPFreeAll();
	return -1;
      }

    /* Data structure with the allocated memory */
    DMANODE *the_event = dmaPGetItem(vmeIN);

    /*
      Physical Address of the_event->data[0]
       - used for programming DMA Destination Address
    */
    printf(" PhysMemBase = 0x%lx\n", the_event->physMemBase);

    /* Insert some data here */
    int32_t ii = 0, len = 0;

#ifdef EX1
    /*
       example 1: write directly to data array
    */

    the_event->data[0] = 0xda000000 + MAX_DATA;
    while(ii < (MAX_DATA + 1))
      {
	the_event->data[1+ii] = ii;
	ii++;
      }
    the_event->data[ii++] = 0xda0000ff;
    len = ii;

#else
    /*
       example 2: bump a pointer to next array element each time a data word is written
    */

    /* Pointer to the first element in the DMA buffer */
    uint32_t *dma_dabufp = (uint32_t *) &(the_event->data[0]);

    *dma_dabufp++ = 0xda000022;
    for(ii=0; ii<MAX_DATA; ii++)
      {
	*dma_dabufp++ = ii;
      }
    *dma_dabufp++ = 0xda0000ff;

    /* Data length = (where the pointer is now) - (where it started)  */
    len = ((u_long)(dma_dabufp) - (u_long)(&the_event->data[0]))>>2;

#endif
    printf("len = %d\n", len);

    for(ii = 0; ii < len; ii++)
      {
	if((ii%5) == 0)
	  printf("\n ");

	printf(" 0x%08x ", the_event->data[ii]);
      }
    printf("\n\n");

    dmaPStatsAll();

    /* Cleanup before program closes */
    dmaPFreeAll();
    dmaPStatsAll();


    exit(0);
}

/*
  Local Variables:
  compile-command: "make -k dmaPListSimple "
  End:
*/
