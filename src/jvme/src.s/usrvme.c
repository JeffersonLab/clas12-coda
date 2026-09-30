
/* vxWorks-like interface: avoids dmaPList and makes vxWorks-compartible API */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "cmem/cmem_rcc.h"
//#include "dmaPList.h"
#include "jvme.h"


static u_long physMemBase;
static u_long userMemBase;
static int32_t memSize = 0x100000; /* must be enough to take maximum event size from entire VME crate*/

/* if using externally allocated destination memory, attributes will be modified; following will
be used to save default values so we can go back to our memory */
static u_long physMemBaseSave;
static u_long userMemBaseSave;
static int32_t memSizeSave;

static int channel = 0; /* there are 2 DMA engines, channel can be 0 or 1 */


void
usrVmeDmaSetMemSize(int size)
{
  memSize = size;
  printf("usrVmeDmaSetMemSize: set memSize to 0x%08x (%d MB)\n",memSize,memSize/1024/1024);
  return;
}

int
usrVmeDmaGetMemSize()
{
  printf("usrVmeDmaGetMemSize: memSize = 0x%08x (%d MB)\n",memSize,memSize/1024/1024);
  return(memSize);
}



int
usrVmeDmaSetChannel(int chan)
{
  if(chan<0 || chan>1)
  {
    printf("ERROR: illegal chan=%d, can be 0 or 1\n",chan);
    return(-1);
  }

  channel = chan;
  printf("INFO: set channel=%d\n",channel);

  return(0);
}

int
usrVmeDmaGetChannel()
{
  printf("INFO: channel=%d\n",channel);
  return(channel);
}










void
usrVmeDmaInit()
{
  DMA_CMEM_ID cmemPart;

  cmemPart = dmaCMemAlloc("dma_hallb",memSize); /*see jvmeDmaAllocLLBuffer*/
  if(cmemPart == NULL)
  {
    printf("%s: ERROR Allocating Memory Handler for DMA\n", __func__);
    exit(1);
  }
  else
  {
    printf("%s: Allocated DMA Memory Handler at 0x%08x\n",__func__, cmemPart);
  }

  printf("usrVmeDmaInit: requested memSize=0x%08x\n",memSize);fflush(stdout);
  memSize = cmemPart->size;
  printf("usrVmeDmaInit: allocated memSize=0x%08x bytes\n",memSize);fflush(stdout);

  physMemBase = cmemPart->paddr;
  userMemBase = cmemPart->vaddr;

  printf("usrVmeDmaInit: userMemBase = 0x%lx, physMemBase = 0x%lx, memSize = %d bytes\n",
    userMemBase, physMemBase, memSize);fflush(stdout);

  memset((u_long *)userMemBase,0,memSize);

  /* save memory attributes */
  physMemBaseSave = physMemBase;
  userMemBaseSave = userMemBase;
  memSizeSave = memSize;

  printf("usrVmeDmaInit: userMemBase = 0x%lx, physMemBase = 0x%lx, memSize = %d bytes\n",userMemBase, physMemBase, memSize);

  return;
}


void
usrVmeDmaMemory(u_long *pMemBase, u_long *uMemBase, int32_t *mSize)
{
  *pMemBase = physMemBase;
  *uMemBase = userMemBase;
  *mSize = memSize;

  return;
}



static unsigned int addrType1;
static unsigned int dataType1;
static unsigned int sstMode1;

void
usrVmeDmaGetConfig(unsigned int *addrType, unsigned int *dataType, unsigned int *sstMode)
{
  *addrType = addrType1;
  *dataType = dataType1;
  *sstMode = sstMode1;

  return;
}

void
usrVmeDmaSetConfig(unsigned int addrType, unsigned int dataType, unsigned int sstMode)
{
  int i;

  addrType1 = addrType;
  dataType1 = dataType;
  sstMode1 = sstMode;


  int32_t retVal = OK;
  retVal = vmeDmaConfig(addrType,dataType,sstMode);

  /* LL config was here */

  return;
}



/* modified jlabgefDmaSend() - AUGHTUNG: PARAMETERS ARE SWAPPED !!! */
/*
 vmeAdrs - VME user address (phys address)
 locAdrs - MEM user address (will be converted to phys address inside that routine)
 nbytes - size of the DMA Transfer in bytes
 */
int
usrVme2MemDmaStart(uint32_t vmeAdrs, u_long locAdrs, int32_t size)
{
  int32_t status = OK;
  u_long offset = 0, physAdrs = 0;
  int32_t nbytes=0;

  /* Local addresses (in Userspace) need to be translated to the physical memory */
  /* Here's the offset between current buffer position and the event head */
  offset = locAdrs - userMemBase;
  physAdrs = physMemBase + offset;

  //printf("\nusrVme2MemDmaStart: locAdrs     = 0x%lx   userMemBase = 0x%lx\n",locAdrs, userMemBase);
  //printf("usrVme2MemDmaStart: physMemBase = 0x%lx   offset      = 0x%lx\n\n",physMemBase,offset);


  //sergey: was nbytes = size * 4; ???
  nbytes = size;


#if 0
  /* Calculate the bytes left in this buffer */
  nbytes = the_event->part->size - sizeof(DMANODE);
  nbytes = (nbytes - ((u_long)dma_dabufp - (u_long)&(the_event->length)));

  /* Make sure nbytes is realistic */
  if(nbytes<0)
  {
    printf("%s: ERROR: Space left in buffer is less than zero (%d). Quitting\n",__func__,nbytes);
    return ERROR;
  }

  /* Check the specified "size" vs. size left in buffer */
  /* ... if size==0, just use the space left in the buffer */
  if(size>nbytes)
  {
    printf("%s: WARN: Specified number of DMA bytes (%d) is greater than \n",__func__,size);
    printf("\tthe space left in the buffer (%d).  Using %d\n",nbytes,nbytes);
  }
  else if( (size !=0) && (size<=nbytes) )
  {
    nbytes = size;
  }
#endif

  //printf("\nusrVme2MemDmaStart: calling vmeDmaSendPhys: physAdrs = 0x%lx, vmeAdrs = 0x%x, nbytes = %d bytes\n\n",physAdrs,vmeAdrs,nbytes);

  status = vmeDmaSendPhys(physAdrs, vmeAdrs, nbytes);

  return(status);
}

int
usrVme2MemDmaDone()
{
  int32_t retVal = OK;

  retVal = vmeDmaDone();

  return(retVal);
}

int
usrVme2MemDmaListSet(unsigned int *vmeAddr, u_long locAddrBase, unsigned int *dmaSize, unsigned int numt)
{
  int32_t rval = 0;

  rval = vmeDmaSetupLL(locAddrBase, vmeAddr, dmaSize, numt);

  return(rval);
}

void
usrVmeDmaListStart()
{ 
  int32_t rval = 0;

  rval = vmeDmaSendLL();

  //return rval;
}

unsigned int
usrDmaLocalToVmeAdrs(unsigned long int locAdrs)
{
  uint32_t retVal=OK;

  retVal = vmeDmaLocalToVmeAdrs(locAdrs);

  return(retVal);
}

void
usrVmeDmaShow()
{
  printf("\n    usrVmeDmaShow (channel %d):\n",channel);

#if 0

  LOCK_TSI;

  printf("control                       [dctl]  = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].dctl));
  printf("status                        [dsta]  = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].dsta));

  printf("Current source address (high) [dcsau] = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].dcsau));
  printf("Current source address (low)  [dcsal] = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].dcsal));

  printf("Current dest address (high)   [dcdau] = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].dcdau));
  printf("Current dest address (low)    [dcdal] = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].dcdal));

  printf("Current link address (high)   [dclau] = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].dclau));
  printf("Current link address (low)    [dclal] = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].dclal));

  printf("source address (high)         [dsau]  = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].dsau));
  printf("source address (low)          [dsal]  = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].dsal));

  printf("dest address (high)           [ddau]  = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].ddau));
  printf("dest address (low)            [ddal]  = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].ddal));

  printf("source attributes             [dsat]  = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].dsat));
  printf("destination attributes        [ddat]  = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].ddat));

  printf("next link address (high)      [dnlau] = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].dnlau));
  printf("next link address (low)       [dnlal] = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].dnlal));

  printf("count (byte)                  [dcnt]  = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].dcnt));
  printf("2eSST broadcast select        [ddbs]  = 0x%08x\n",LSWAP(pTempe->lcsr.dma[channel].ddbs));


  UNLOCK_TSI;

#endif

  printf("\n");
}
