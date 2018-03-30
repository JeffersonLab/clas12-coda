
/* bigbuf.c - library for the memory allocation system */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "circbuf.h"

#define logMsg printf

#define BB_LOCK   pthread_mutex_lock(&bbp->bb_lock)
#define BB_UNLOCK pthread_mutex_unlock(&bbp->bb_lock)

#define LSWAP(x)        ((((x) & 0x000000ff) << 24) | \
                         (((x) & 0x0000ff00) <<  8) | \
                         (((x) & 0x00ff0000) >>  8) | \
                         (((x) & 0xff000000) >> 24))

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/********** BIG BUFFERS MANAGEMENT PACKAGE **********************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#include "bigbuf.h"

/*
#define DEBUG
*/

/* returns pool id */

BIGBUF *
bb_new(int id, int nbufs, int nbytes)
{
  BIGBUF *bbp;
  int i;

  /* check parameters */
  if(nbufs > NBIGBUFFERS)
  {
    logMsg("bb_new: ERROR: nbufs=%d (must be <= %d\n",
      nbufs,NBIGBUFFERS,3,4,5,6);
    return(0);
  }

  /* allocate structure */
  bbp = (BIGBUF *) malloc(sizeof(BIGBUF));
  if(bbp == NULL)
  {
    logMsg("bb_new: ERROR: cannot allocate memory for 'bbp'\n",
      1,2,3,4,5,6);
    return(0);
  }
  else
  {
    bzero((void *)bbp,sizeof(BIGBUF));
  }

  /* allocate buffers */
  bbp->nbufs = nbufs;
  bbp->nbytes = nbytes;
  for(i=0; i<bbp->nbufs; i++)
  {
    bbp->data[i] = (unsigned int *) malloc(bbp->nbytes);
    if(bbp->data[i] == NULL)
    {
      logMsg("bb_new: ERROR: buffer allocation FAILED\n",1,2,3,4,5,6);
      return(0);
    }
  }

  /* initialize semaphores */
  pthread_mutex_init(&bbp->bb_lock, NULL);

  /* initialize index */
  bbp->write = 1;
  bbp->read = 0;

  /* reset cleanup condition */
  bbp->cleanup = 0;

  bbp->id = id;

  logMsg("bb_new: 'big' buffer id=%d created (addr=0x%08x, %d bufs, %d size)\n",
		 id,bbp,bbp->nbufs,bbp->nbytes,5,6);

  return(bbp);
}





/*************/
/* GEF staff */

#ifdef Linux_vme

#include "../../rol/jvme/jvme.h"
#include "gef/gefcmn_vme.h"

GEF_VME_BUS_HDL vmeHdl;

static int n_dma_hdl = 0;
GEF_VME_DMA_HDL dma_hdl[1024];

#define UINT32 unsigned int


/*similar to the usrVmeDmaInit() from usrvme.c*/
void
bb_InitChunk(BIGBUF *bbp, int n)
{
  unsigned int offset;
  GEF_STATUS status;
  GEF_MAP_PTR mapPtr;

  if(vmeHdl==NULL)
  {
    printf("bb_InitChunk ERROR: vmeHdl undefined.\n");
    return;
  }
 /* Parameters:
 *    <GEF_VME_BUS_HDL> vmeHdl   - (I) Handle allocated by <gefVmeOpen>.
 *    GEF_UINT32 size         - (I) size of the buffer.
 *    <GEF_VME_DMA_HDL> *dma_hdl - (O) DMA handle of type <GEF_VME_DMA_HDL>.
 *    void **addr                - (O) User process address to allocated DMA Buffer.
 */
  status = gefVmeAllocDmaBuf (vmeHdl, bbp->nbytes, &dma_hdl[n_dma_hdl], &mapPtr);
  if(status != GEF_STATUS_SUCCESS) 
  {
    printf("bb_InitChunk ERROR: gefVmeAllocDmaBuf returned 0x%x\n",status);
    return;
  }

  bbp->data[n] = (unsigned int *)mapPtr;
  memset((char *)bbp->data[n],0,bbp->nbytes);

  bbp->userMemBase[n] = (unsigned int)bbp->data[n];
  bbp->physMemBase[n] = (unsigned int)dma_hdl[n_dma_hdl]->phys_addr;

  offset = bbp->userMemBase[n] - bbp->physMemBase[n];

  printf("bb_InitChunk[%2d] (n_dma_hdl=%2d): userMem=0x%08x, physMem=0x%08x, size=%d bytes (offset=0x%08x)\n",
		 n, n_dma_hdl, bbp->userMemBase[n], bbp->physMemBase[n], bbp->nbytes, offset);

  n_dma_hdl ++;

  return;
}


/* big buffers for ROL1 will be in the special area for VME->MEM DMA */

BIGBUF *
bb_new_rol1(int id, int nbufs, int nbytes)
{
  BIGBUF *bbp;
  int i, status;
  int i1, i2, i3;

  /* check parameters */
  if(nbufs > NBIGBUFFERS)
  {
    logMsg("bb_new_rol1: ERROR: nbufs=%d (must be <= %d\n",
      nbufs,NBIGBUFFERS,3,4,5,6);
    return(0);
  }

  /* allocate structure */
  bbp = (BIGBUF *) malloc(sizeof(BIGBUF));
  if(bbp == NULL)
  {
    logMsg("bb_new_rol1: ERROR: cannot allocate memory for 'bbp'\n",
      1,2,3,4,5,6);
    return(0);
  }
  else
  {
    bzero((void *)bbp,sizeof(BIGBUF));
  }

  /* allocate buffers */
  bbp->nbufs = nbufs;
  bbp->nbytes = nbytes;





  /* open dma memory driver */
  status = gefVmeOpen(&vmeHdl);
  if (status != GEF_STATUS_SUCCESS) 
  {
    printf("bb_new_rol1: gefVmeOpen failed: %x\n",status);
    return(0);
  }
  /* Turn off the Debug Flags, by default */
  gefVmeSetDebugFlags(vmeHdl,0x0);




  printf("bb_new_rol1: allocate %d chunks %d(0x%08x) bytes each\n",bbp->nbufs,bbp->nbytes,bbp->nbytes);
  for(i=0; i<bbp->nbufs; i++)
  {
    bbp->data[i] == NULL;
    bb_InitChunk(bbp, i);
    if(bbp->data[i] == NULL)
    {
      logMsg("bb_new_rol1: ERROR: buffer allocation FAILED\n",1,2,3,4,5,6);
      return(0);
    }
  }

  /* initialize semaphores */
  pthread_mutex_init(&bbp->bb_lock, NULL);

  /* initialize index */
  bbp->write = 1;
  bbp->read = 0;

  /* reset cleanup condition */
  bbp->cleanup = 0;

  bbp->id = id;

  logMsg("bb_new_rol1: 'big' buffer id=%d created (addr=0x%08x, %d bufs, %d size)\n",
		 id,bbp,bbp->nbufs,bbp->nbytes,5,6);

  return(bbp);
}


unsigned int *
bb_get_usermembase(BIGBUF **bbh)
{
  BIGBUF *bbp = *bbh;

  return(bbp->userMemBase[bbp->write]);
}

unsigned int *
bb_get_physmembase(BIGBUF **bbh)
{
  BIGBUF *bbp = *bbh;

  return(bbp->physMemBase[bbp->write]);
}

void
bb_dma_free()
{
  int i, status;

  for(i=0; i<n_dma_hdl; i++)
  {
    status = gefVmeFreeDmaBuf(dma_hdl[i]);
    if (status != GEF_SUCCESS)
    {
      printf("bb_delete1: ERROR: gefVmeFreeDmaBuf[%2d] returned %x\n",i,status);
      return;
    }
    else
	{
      printf("bb_delete1: gefVmeFreeDmaBuf[%2d] has been called\n",i);
	}
  }
}


void
bb_delete1(BIGBUF **bbh)
{
  BIGBUF *bbp = *bbh;
  int i, status;

printf("bb_delete1 0: 0x%08x\n",bbh);fflush(stdout);

  if((bbh == NULL)||(*bbh == NULL)) return;

  pthread_mutex_unlock(&bbp->bb_lock);
  pthread_mutex_destroy(&bbp->bb_lock);

printf("bb_delete1 5\n");fflush(stdout);

  /* free buffers 
  for(i=0; i<bbp->nbufs; i++)
  {
printf("bb_delete1 [%d]\n",i);fflush(stdout);
    free(bbp->data[i]);
printf("bb_delete1 (%d)\n",i);fflush(stdout);
  }
  */
  bb_dma_free();

  /* free 'bbp' structure */
printf("bb_delete1 6\n");fflush(stdout);
  free(bbp);
printf("bb_delete1 7\n");fflush(stdout);



  return;
}

#endif /* Linux_vme */

/* end of GEF staff */
/********************/
















/* */

void
bb_delete(BIGBUF **bbh)
{
  BIGBUF *bbp = *bbh;
  int i;

printf("bb_delete 0: 0x%08x\n",bbh);fflush(stdout);

  if((bbh == NULL)||(*bbh == NULL)) return;

  pthread_mutex_unlock(&bbp->bb_lock);
  pthread_mutex_destroy(&bbp->bb_lock);

printf("bb_delete 5\n");fflush(stdout);

  /* free buffers */
  for(i=0; i<bbp->nbufs; i++)
  {
printf("bb_delete [%d]\n",i);fflush(stdout);
    free(bbp->data[i]);
printf("bb_delete (%d)\n",i);fflush(stdout);
  }

  /* free 'bbp' structure */
printf("bb_delete 6\n");fflush(stdout);
  free(bbp);
printf("bb_delete 7\n");fflush(stdout);
}


/* */

void
bb_cleanup(BIGBUF **bbh)
{
  BIGBUF *bbp = *bbh;
  int i;

printf("bb_cleanup 0: 0x%08x\n",bbh);fflush(stdout);

  if((bbh == NULL)||(*bbh == NULL)) return;

printf("bb_cleanup 1: 0x%08x\n",bbp);fflush(stdout);

  bbp->cleanup = 1;
printf("bb_cleanup 2\n");fflush(stdout);

  return;
}

void
bb_init(BIGBUF **bbh)
{
  BIGBUF *bbp = *bbh;
  int i;

  if((bbh == NULL)||(*bbh == NULL))
  {
    logMsg("bb_init: ERROR: bbh=0x%08x *bbh=0x%08x\n",bbh,*bbh,3,4,5,6);
    return;
  }

  /* initialize index */
  bbp->write = 1;
  bbp->read = 0;

  /* reset cleanup condition */
  bbp->cleanup = 0;


  /* need that ? */
  printf("bb_init: clear memory for %d buffers\n",bbp->nbufs);fflush(stdout);
  for(i=0; i<bbp->nbufs; i++)
  {
    printf("bb_init: buffer[%d]: clear %d bytes starting from 0x%08x\n",
		   i,bbp->nbytes,bbp->data[i]);fflush(stdout);
    memset(bbp->data[i],0,bbp->nbytes);
  }

  return;
}













/*********************************************************************************************/
/*********************************************************************************************/
/********************************* start mutex-protected section *****************************/
/*********************************************************************************************/
/*********************************************************************************************/
/*********************************************************************************************/



/* write method: gets free buffer from the 'pool' for writing; */
/* it waits for available buffer and returns buffer pointer */

unsigned int *
bb_write_(BIGBUF **bbh, int flag)
{
  BIGBUF *bbp = *bbh;
  int icb;

  if((bbh == NULL)||(*bbh == NULL))
  {
    logMsg("[%d] bb_write ERROR 1\n",bbp->id,2,3,4,5,6); 
    return(NULL);
  }

  if(bbp->cleanup)
  {
    logMsg("[%d] bb_write 1: return(NULL) on bbp->cleanup=%d condition\n",
      bbp->id,bbp->cleanup,3,4,5,6); 
    return(NULL);
  }

  BB_LOCK;

#ifdef DEBUG
  printf("[%d] bb_write (in):          write=%d read=%d\n",bbp->id,bbp->write,bbp->read);fflush(stdout);
#endif

  /* try to take next (empty) buffer; if not available - sleep and try again */
  icb = (bbp->write + 1) % bbp->nbufs;  

  while(icb == bbp->read)
  {
    if(bbp->cleanup)
    {
      logMsg("[%d] bb_write: return(NULL) on bbp->cleanup=%d condition\n",
        bbp->id,bbp->cleanup,3,4,5,6);
      BB_UNLOCK;
      return(NULL);
	}

/*printf("[%d] bb_write: waiting for buffer (write=%d read=%d) unlock \n",bbp->id,bbp->write,bbp->read);*/
    BB_UNLOCK;
	
    if(flag) return(NULL);
	
	usleep(10000); /* was 100000 */
	
    BB_LOCK;
/*printf("[%d] bb_write: waiting for buffer (write=%d read=%d) lock\n",bbp->id,bbp->write,bbp->read);*/
	
  }

  bbp->write = icb;

#ifdef DEBUG
  printf("[%d] bb_write (out):         write=%d read=%d\n",bbp->id,bbp->write,bbp->read);fflush(stdout);
#endif

  BB_UNLOCK;

  return(bbp->data[icb]);
}

unsigned int *
bb_write(BIGBUF **bbh)
{
  return(bb_write_(bbh,0));
}

unsigned int *
bb_write_nodelay(BIGBUF **bbh)
{
  return(bb_write_(bbh,1));
}





unsigned int *
bb_write_current(BIGBUF **bbh)
{
  BIGBUF *bbp = *bbh;
  int icb;

  if((bbh == NULL)||(*bbh == NULL)) return(NULL);

  
  BB_LOCK;
  
#ifdef DEBUG
  printf("[%d] bb_write_current (in):  write=%d read=%d\n",bbp->id,bbp->write,bbp->read);fflush(stdout);
#endif

  icb = bbp->write;

#ifdef DEBUG
  printf("[%d] bb_write_current (out): write=%d read=%d\n",bbp->id,bbp->write,bbp->read);fflush(stdout);
#endif
  
  BB_UNLOCK;
  
  return(bbp->data[icb]);
}












/* read method: gets full buffer from the 'pool' for reading; */
/* it waits for available buffer and returns buffer pointer */

unsigned int *
bb_read(BIGBUF **bbh)
{
  BIGBUF *bbp = *bbh;
  int icb;

  if((bbh == NULL)||(*bbh == NULL)) return(NULL);
  if(bbp->cleanup)
  {
    logMsg("[%d] bb_read 1: return(NULL) on bbp->cleanup=%d condition\n",
      bbp->id,bbp->cleanup,2,3,4,5,6); 
    return(NULL);
  }

  BB_LOCK;

#ifdef DEBUG
  printf("[%d] bb_read  (in):          write=%d read=%d\n",bbp->id,bbp->write,bbp->read);fflush(stdout);
#endif

  /* try to get next (full) buffer; if not available - sleep */
  icb = (bbp->read + 1) % bbp->nbufs;
  
  while(icb == bbp->write)
  {
    if(bbp->cleanup)
    {
      logMsg("[%d] bb_read: return(NULL) on bbp->cleanup=%d condition\n",
        bbp->id,bbp->cleanup,3,4,5,6); 
      BB_UNLOCK;
      return(NULL);
	}
    BB_UNLOCK;

	/*sleep(1);*/usleep(100000);
    BB_LOCK;
  }


  /* set 'read' pointer to the next buffer */
  bbp->read = icb;

#ifdef DEBUG
  printf("[%d] bb_read  (out):         write=%d read=%d\n",bbp->id,bbp->write,bbp->read);fflush(stdout);
#endif

  BB_UNLOCK;

  return(bbp->data[icb]);
}


/*********************************************************************************************/
/*********************************************************************************************/
/*********************************************************************************************/
/********************************** end mutex-protected section ******************************/
/*********************************************************************************************/
/*********************************************************************************************/






unsigned int *
bb_check(unsigned int *data)
{
  int j, k, nev, lenev, lenbuf, iev, llenw;
  unsigned int *buf, magic;

  magic = data[BBIFD];
  if(magic == 0x01020304)
  {
    printf("SWAP (0x%08x)\n",magic);
    llenw = LSWAP(data[BBIWORDS]);
	printf("bb_check: llenw=%d\n",llenw);
    bufferSwap(data,llenw);
  }

  printf("BIGBUF: buflen=%d, bufnum=%d, rocid=%d, nev=%d, magic=%d, end=%d, nhead=%d\n",
         data[BBIWORDS],
         data[BBIBUFNUM],
		 data[BBIROCID],
		 data[BBIEVENTS],
		 data[BBIFD],
		 data[BBIEND],
		 data[BBHEAD]);

  if(data[BBIBUFNUM] < 0)
  {
    return(0);
  }

  lenbuf = 0;
  nev = data[BBIEVENTS];
  buf = data + BBHEAD;
  for(j=0; j<nev; j++) 
  {
    lenev = buf[0]+1; 
    lenbuf += lenev;
    iev = ( (((buf[1]>>16)&0xff)==0) ? 0 : 1 ); 

    printf("bb_check: nev=%d -> 0x%08x 0x%08x -> lenev=%d, iev=%d, so far lenbuf=%d\n",j,buf[0],buf[1],lenev,iev,lenbuf);

    for(k=0; k<256/*lenev*/; k++)
	{
      printf(" [%5d]0x%08x",k,buf[k]);
      if(((k+1)%8)==0) printf("\n");
	}

    buf += lenev; 
  } 

  return(0);
}


#define UINT32 unsigned int

int
usrMem2MemDmaCopy(UINT32 chan, UINT32 *sourceAddr, UINT32 *destAddr,
                  UINT32 nbytes)
{
  memcpy((char *)destAddr, (char *)sourceAddr,nbytes);

  return(0);
}

int
usrMem2MemDmaStart(UINT32 chan, UINT32 *sourceAddr, UINT32 *destAddr,
                  UINT32 nbytes)
{
  memcpy((char *)destAddr, (char *)sourceAddr,nbytes);

  return(0);
}

int
usrDmaDone(UINT32 chan)
{
  return(0);
}
