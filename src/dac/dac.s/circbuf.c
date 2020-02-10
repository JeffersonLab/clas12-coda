
/* circbuf.c - circular buffer library */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "circbuf.h"

#undef DEBUG

#define MIN(x,y)    ( (x) < (y) ? (x) : (y) )
#define NFBUF(r,w)  ( (w) >= (r) ? ((w)-(r)) : ((w)-(r)+QSIZE) )

#define READ_LOCK    pthread_mutex_lock(&cbp->read_lock)
#define WRITE_LOCK   pthread_mutex_lock(&cbp->write_lock)

#define READ_UNLOCK  pthread_mutex_unlock(&cbp->read_lock)
#define WRITE_UNLOCK pthread_mutex_unlock(&cbp->write_lock)

#define READ_WAIT    pthread_cond_wait(&cbp->read_cond, &cbp->read_lock)
#define WRITE_WAIT   pthread_cond_wait(&cbp->write_cond, &cbp->write_lock)




#define READ_SIGNAL  pthread_cond_signal(&cbp->read_cond)
#define WRITE_SIGNAL pthread_cond_signal(&cbp->write_cond)


/*#define READ_SIGNAL_ALL  for(ii=0; ii<nrocs; ii++) {ccc = cba[ii]; pthread_cond_signal(&ccc->read_cond);}*/
#define READ_SIGNAL_ALL  for(ii=0; ii<nrocs; ii++) {ccc = cba[ii]; pthread_cond_broadcast(&ccc->read_cond);}



static CIRCBUF rocqueues[MAX_ROCS]; /* here index from 0 to nrocs */


/* cb_init must be called in the very beginning of Prestart
to cleanup everything in circular buffer package; we are calling it from 'debopenlinks()' */
CIRCBUF *
cb_init(int roc, char *name, char *parent)
{
  CIRCBUF *cbp;
  int i;
  char temp[100];

  cbp = (CIRCBUF *) &rocqueues[roc];
  memset((void *)cbp,0,sizeof(CIRCBUF));

  cbp->roc = roc;
  sprintf(temp,"%s[%02d]",name,cbp->roc);
  strncpy(cbp->name, temp, 99);
  strncpy(cbp->parent, parent, 99);

  pthread_mutex_init(&cbp->read_lock, NULL);
  pthread_cond_init(&cbp->read_cond, NULL);

  pthread_mutex_init(&cbp->write_lock, NULL);
  pthread_cond_init(&cbp->write_cond, NULL);

  cbp->write = cbp->read = 0;

  for(i=0; i<QSIZE; i++) cbp->nattach[i] = 0;
  for(i=0; i<NTHREADMAX; i++) cbp->nbuf[i] = 0;
  printf("cb_init(): circular buffer for roc=%d initialized\n",roc);
  printf("cb_init(): circular buffer for roc=%d '%s' initialized by '%s' (addr=0x%08x)\n",
		 roc,cbp->name,cbp->parent,cbp);
  fflush(stdout);

  return(cbp);
}



/* here 'roc' from 0 to nrocs */
void
cb_delete(int roc)
{
  CIRCBUF *cbp;
  int i;

  cbp = (CIRCBUF *) &rocqueues[roc];

  printf("cb_delete roc=%d\n",roc);fflush(stdout);

  /* set flag to initiate deleting process */
  cbp->deleting = 1;

  /* broadcast to PUTs and GETs which can be waiting on locks;
  after waking up they suppose to check 'deleting' flag before
  doing anything else, and behave appropriately */
  pthread_cond_broadcast(&cbp->read_cond);
  pthread_cond_broadcast(&cbp->write_cond);

  return;
}



/*******************************************************
 * put_cb_data() puts new data on the queue.
 * If the queue is full, it waits until there is room.
 *******************************************************/


/* 'fd' is used for printing purposes only */
int
put_cb_data(int fd, CIRCBUF **cbh, void *data)
{
  int icb, i;
  CIRCBUF *cbp = *cbh;
  unsigned int *buf;

#ifdef DEBUG
  printf("[%2d] put_cb_data(reached): cbp=0x%08x\n",fd,cbp);
  fflush(stdout);
#endif
  if((cbh == NULL)||(*cbh == NULL)) return(-1);

  /*sergey: temporary until resolved (seems happens running EB on vme controllers)*/
  if(cbp <(CIRCBUF *)100000)
  {
    printf("[%2d] PUT: ERROR return on chb<100000\n",fd);fflush(stdout);
    /*return(-11);*/
    exit(1);
  }


#ifdef DEBUG
  printf("[%2d] PUT: 2\n",fd);
  fflush(stdout);
  printf("[%2d] PUT: 2 cbp=0x%08x\n",fd,cbp);
  fflush(stdout);
#endif

  if(cbp->deleting)
  {
    printf("[%2d] PUT: fifo is being emptied !\n",fd);
    fflush(stdout);
    return(-1);
  }

#ifdef DEBUG
  printf("[%2d] PUT: 21\n",fd);
  fflush(stdout);
#endif


  /****************************/
  /* get current buffer index */
  icb = cbp->write;
#ifdef DEBUG
  printf("[%2d] PUT: 22\n",fd);
  fflush(stdout);
  printf("[%2d] PUT:: current icb=%d (obtained from cbp->write=%d; cbp->read=%d)\n",fd,
      icb,cbp->write,cbp->read);
  fflush(stdout);
#endif

  /* print some F's if '-1' (?) */
  if(cbp->data[icb] == (void *)-1) {printf("[%2d] PUT: FFFFFFFFFFFFFFFFFF\n",fd); fflush(stdout);}

#ifndef NOALLOC
  /* release previously used buffer */
  if((cbp->data[icb] != NULL) && (cbp->data[icb] != (void *)-1))
  {
#ifdef DEBUG
    printf("[%2d] PUT: release [%1d] 0x%08x\n",fd,icb,cbp->data[icb]);
    fflush(stdout);
#endif
    cfree(cbp->data[icb]);
  }
#endif

  /**********************************************************/
  /* assign pointer to the data recieved as input parameter */
  cbp->data[icb] = data;
#ifdef DEBUG
  printf("[%2d] PUT: got icb=%2d at data=0x%08x (int=%d uint=%d)\n",
    fd,icb,cbp->data[icb],(int)cbp->data[icb],(unsigned int)cbp->data[icb]);
  fflush(stdout);
#endif

  /*****************************************************************/
  /* if data pointer is valid, get some info and set write pointer */
  if((long)cbp->data[icb] != -1/*(int)data > 0 || (int)data < -120000000*/) /* can be -1 which means no buffer !!! */
  {
    int lll;
    buf = (unsigned int *)data;
    lll = buf[BBIWORDS]<<2;
    cbp->rocid = buf[BBIROCID];         /* ROC id */
    cbp->nevents[icb] = buf[BBIEVENTS]; /* the number of events in buffer */

	/* some basic checks */
	if(buf[BBIEVENTS]<=0)
	{
      printf("[%2d] PUT: DATA ERROR: nev=%d (rocid=%d); beginning of the buffer:\n",fd,buf[BBIEVENTS],buf[BBIROCID]);
      {
        int i;
        unsigned int *buff = (unsigned int *)data;
        for(i=0; i<32; i+=8)
          printf("[%2d] PUT: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",fd,
            buff[i],buff[i+1],buff[i+2],buff[i+3],buff[i+4],buff[i+5],buff[i+6],buff[i+7]);
      }
      fflush(stdout);
	}

    /* &buf[BBHEAD] - start of first event, contains event length in words */
    buf += BBHEAD;
    cbp->evptr1[icb] = buf; /* pointer to the first event in buffer */

#ifdef DEBUG
    printf("[%2d] PUT[%2d]: nevents=%d, buf=0x%08x, 1st 0x%08x, len=0x%08x (end=0x%08x)\n",fd,
    icb,cbp->nevents[icb],data,cbp->evptr1[icb],lll,(int)buf+lll);
	
    {
      int i;
      unsigned int *buff = (unsigned int *)data;
      for(i=0; i<32; i+=8)
        printf("[%2d] PUT: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",fd,
          buff[i],buff[i+1],buff[i+2],buff[i+3],buff[i+4],buff[i+5],buff[i+6],buff[i+7]);
    }
    fflush(stdout);
	
    printf("[%2d] PUT: icb=%2d contains %d events\n",fd,icb,cbp->nevents[icb]);
    fflush(stdout);
#endif

    /*************************************************************/
    /* check if next buffer has been read already; if not - wait */
#ifdef DEBUG
    printf("[%2d] PUT: read_locking, icb=%d, will increment it\n",fd,icb);fflush(stdout);
#endif
    READ_LOCK;
    icb = (icb + 1) % QSIZE;
    while(icb == cbp->read)
    {
/*prints all the time
      printf("[%2d] PUT: wait while icb=%d == cbp->read=%d -------\n",fd,icb,cbp->read);
*/
      READ_WAIT;
    }
    READ_UNLOCK;
#ifdef DEBUG
    printf("[%2d] PUT: read_unlocked for previous icb, write_locking icb=%d\n",fd,icb);fflush(stdout);
#endif

    /***************************************************************************/
    /* now we know that next buffer was read out, so set 'write' pointer to it */
    WRITE_LOCK;
#ifdef DEBUG
    printf("[%2d] PUT: inside write_lock, icb=%d\n",fd,icb);fflush(stdout);
#endif
	cbp->write = icb;

    /******************************************/
    /* let a waiting reader know there's data */
    WRITE_SIGNAL;
    WRITE_UNLOCK;
#ifdef DEBUG
    printf("[%2d] PUT: came from write_unlock, icb=%d\n",fd,icb);fflush(stdout);
#endif
  }
  else
  {
#ifdef DEBUG
    printf("[%2d] PUT: BLYA: ROC >%s< data=%d -> set cbp->nevents[%d]=-1\n",fd,
                cbp->name,(int)data,icb,cbp->nevents[icb]);
    fflush(stdout);
#endif
    cbp->nevents[icb] = -1; /* need it in cb_events_get() */
  }

#ifdef DEBUG
  printf("[%2d] PUT: done: cbp=0x%08x\n",fd,cbp);
  fflush(stdout);
#endif

  return(0);
}


/*
 * get_cb_count() gets the number of filled circular buffers.
 */

/* USED TO FLASH INPUT STREAMS ONLY NOW !!! */

int
get_cb_count(CIRCBUF **cbh)
{
  CIRCBUF *cbp = *cbh;
  int count;
  
  if((cbh == NULL)||(*cbh == NULL)) return(-1);

  READ_LOCK;
  WRITE_LOCK;

  count = NFBUF(cbp->read,cbp->write);
  printf("get_cb_count: count = %d\n",count);
  fflush(stdout);

  READ_UNLOCK;
  WRITE_UNLOCK;

  return(count);
}






/********************/
/* group operations */


/*
   Gets a chunk of event fragments from all circular buffers

   input:  cba[MAX_ROCS] - array of circular buffers
           id - 'id' of the calling process
           nrocs - the number of active rocs (ebp->nrocs from deb_component.c)
           chunk - the number of events requested
   output: **buf - pointers to the events returned
		   *nphys - the number of physics events returned (excluding scaler events)
   return: the number of events returned
*/

#define NPROF1 100


/* fills array of events */ 
#define FILL_EVENT_ARRAY(nev1, nev2, flag)	\
  for(j=nev1; j<nev2; j++) \
  { \
    /*printf("[%1d] INFO(%d): roc %d, j=%d: nw = %d, sync=%d tag=0x%02x typ=0x%02x num=%d\n",id,flag,i,j,buf[0],(buf[1]>>24)&0xff,(buf[1]>>16)&0xff,(buf[1]>>8)&0xff,(buf[1])&0xff);*/ \
    lenev = buf[0]+1; \
    /*memcpy(evptr[j],buf,lenev<<2);*/	evptr[j] = buf; /*sergey: if do actual copy, allocate space in coda_ebc.c !!! */ \
    /*printf("[%1d] INFO(%d): copied %d bytes\n",id,flag,lenev<<2);*/	\
    lenbuf += lenev; /* returned buffer length in words */ \
    if( ((buf[1]>>16)&0xff)==0 )					\
	{ \
      int jj; \
      unsigned int *bbb; \
      printf("[%1d] ERROR(%d): roc %d, nev1=%d nev2=%d j=%d bank tag is zero (header: %d 0x%08x)\n",id,flag,i,nev1,nev2,j,buf[0],buf[1]); \
      /*for(jj=0; jj<j+1; jj++) \											
	  { \
        bbb = evptr[jj]; \
        printf("[%1d] INFO(%d): roc %d, jj=%d: nw = %d, sync=%d tag=0x%02x typ=0x%02x num=%d\n",id,flag,i,jj,bbb[0],(bbb[1]>>24)&0xff,(bbb[1]>>16)&0xff,(bbb[1]>>8)&0xff,(bbb[1])&0xff); \
		}*/ \
	} \
	else \
	{ \
      nevphys ++; \
	} \
    buf += lenev; \
  } \
  cbp->nevents[icb] -= (nev2-nev1); /* update the number of events in buffer */ 




/* here cba[] and evptrr[] index from 0 to nrocs */
int
cb_events_get(CIRCBUF *cba[MAX_ROCS], int id, int nrocs, int chunk,
              unsigned int *evptrr[MAX_ROCS][NCHUNKMAX], int *nphys)
{
  int i, ii, j, nev1, nev2, nevtot, nevbuf, lenev, icb, icb_next, lenbuf, nev;
  int nevphys, itmp, icb_next_available;
  CIRCBUF *cbp, *ccc;
  unsigned int *buf;
  unsigned int **evptr;

#ifdef DEBUG
  printf("[%1d] cb_events_get reached, nrocs=%d, chunk=%d\n",id,nrocs,chunk);
  fflush(stdout);
#endif

  if(nrocs<=0)
  {
    printf("[%1d] cb_events_get: ERROR: nrocs=%d, return no events\n",id,nrocs);
    fflush(stdout);
    return(0);
  }


  /***************************************************************
   first loop - found the minimum number of events
  and compare it with 'chunk'; use the minimum one;
  buffers will be locked and unlocked one by one to get 'nevtot' */

  nevphys = 0;
  nev = chunk;
  for(i=0; i<nrocs; i++)
  {
    cbp = cba[i]; /* pointer to the next fifo */
#ifdef DEBUG
	printf("[%1d] cbp = cba[%d] = 0x%08x (*cbp = 0x%08x)\n",id,i,cbp,*cbp);
    fflush(stdout);
#endif

    /* get first buffer number with valid data
    and the number of the next buffer */
    WRITE_LOCK;
    icb = cbp->read;
#ifdef DEBUG
    printf("[%1d] GET1: icb=%d\n",id,icb);
#endif
    while(icb == cbp->write)
    {
#ifdef DEBUG
      printf("[%1d] GET1: wait (cbp->write=%d) ..............\n",id,cbp->write);
#endif
      WRITE_WAIT;
#ifdef DEBUG
      printf("[%1d] GET1: proceed ..\n",id);
#endif  
      if(cbp->deleting)
      {
        printf("[%1d] GET1: deleting 1 .. %8.8s\n",id,cbp->name);fflush(stdout);
        WRITE_UNLOCK;
        return(-1);
      }
    }
    WRITE_UNLOCK;





    icb_next = (icb + 1) % QSIZE;
#ifdef DEBUG
    printf("[%1d]  GET1: icb_next=%d cbp->write=%d\n",id,icb_next,cbp->write);
#endif
    if(icb_next == cbp->write)
    {
      /*printf("  GET1: next is NOT available (cbp->write=%d)\n",cbp->write);*/
      icb_next_available = 0;
    }
    else
    {
      /*printf("  GET1: next is available (cbp->write=%d)\n",cbp->write);*/
      icb_next_available = 1;
    }


    /**************************************************************************/
    /* wait while there's nothing in the buffer (SHOULD NEVER BE HERE ???!!!) */
    while(cbp->nevents[icb] == 0)
    {
      printf("[%1d] GET1: >%8.8s< NEVER COME HERE - 1 (nevents=0)\n",id,cbp->name);fflush(stdout);
      if(cbp->deleting && (NFBUF(cbp->read,cbp->write) == 0))
      {
        printf("[%1d] GET1: deleting 2 .. %8.8s\n",id,cbp->name);fflush(stdout);
        return(-1);
      }

#ifdef DEBUG
      printf("[%1d] GET1: empty, wait for %s (rocid=%d) (adr=0x%08x) %d\n",
                   id,cbp->name,cbp->rocid,cbp,cbp->nevents[icb]);fflush(stdout);
      printf("[%1d] GET1: #roc=%d\n",id,cbp->rocid);
      fflush(stdout);
#endif
      sleep(1);
#ifdef DEBUG
      printf("[%1d] GET1: itmp=%d\n",id,itmp);fflush(stdout);
#endif
    }


    if((long)cbp->data[icb] != -1 /*(int)cbp->data[icb] > 0 || (int)cbp->data[icb] < -120000000*/ )
    {
      if(icb_next_available)
      {
        if(cbp->nevents[icb_next]<=0)
        {
          printf("[%1d] GET1: ERROR <<<%d>>>\n",
            id,cbp->nevents[icb_next]);
          fflush(stdout);
        }
        nevtot = MIN(chunk,(cbp->nevents[icb]+cbp->nevents[icb_next]));
      }
      else
      {
        nevtot = MIN(chunk,cbp->nevents[icb]);
      }
    }
    else
	{
      printf("[%1d] GET1: ERROR: cbp->data[icb]=%d (0x%08x)\n",id,cbp->data[icb],cbp->data[icb]);
	}
    nev = MIN(nev,nevtot);

    /* return 0 if there's nothing at least in one of the fifo's */
    /* will NEVER happens - we have 'wait' above !!! */
    if(nev == 0)
    {
      printf("[%1d] GET1: NEVER COME HERE 2\n",id);fflush(stdout);
      return(0);
	}
  }
  /* at that point we obtained 'nev' - the least number of events in every rocs */
  /******************************************************************************/


#ifdef DEBUG
  printf("\n[%1d] GET1: =================> obtained nev=%d <====================\n\n",id,nev);fflush(stdout);
#endif


  /******************************************************/
  /* loop again over array of fifo's taking 'nev' events;
  unlock every fifo after processing !!! */
  lenbuf = 0;
  for(i=0; i<nrocs; i++)
  {
    cbp = cba[i]; /* set pointer to the next roc's fifo */
    /*if(cbp != NULL) printf("[%2d] GET2: %8.8s\n",i,cbp->name);*/


    /* release buffers kept by 'id' if any */
    for(j=0; j<cbp->nbuf[id]; j++)
    {
      icb = (cbp->buf1[id]+j) % QSIZE;
      /*printf("      GET2: release icb[%1d]=%d\n",j,icb);*/
      cbp->nattach[icb] --;
      if(cbp->nattach[icb] < 0)
      {
        printf("[%1d][%2d] GET2: ERROR: cbp->nattach[%d]=%d\n",
			   id,i,icb,cbp->nattach[icb]); fflush(stdout);
      }

#ifdef NOALLOC
      /* mark buffer as 'free' if .. */
      if(cbp->nattach[icb]==0) /* .. nobody holds that buffer, and .. */
      {
        if(cbp->nevents[icb]==0) /* .. all events are used from that buffer */
        {
          buf = (unsigned int *)cbp->data[icb];
#ifdef DEBUG
          printf("[%1d][%2d] GET2: free buffer %d (0x%08x)\n",id,i,icb,buf);
          fflush(stdout);
#endif
          buf[0] = 0; /* mark buffer as free */




		  /* send signal to LINK_support ??? */




        }
      }
#endif

    }
    cbp->nbuf[id] = 0;


    /* get first buffer number with valid data
    and the number of the next buffer */
    icb = cbp->read;
    icb_next = (icb + 1) % QSIZE;
    evptr = evptrr[i];


    /* if buffer pointer is greater then zero then process the buffer */ 
    /* -1 means no buffer !!! */ 
    if((long)cbp->data[icb] != -1 /*(int)cbp->data[icb] > 0 || (int)cbp->data[icb] < -120000000*/ ) 
    {
      nevbuf = nev; 

if(cbp->nevents[icb] < 0) 
{ 
printf("%s ????? \n",cbp->name);fflush(stdout); 
printf("%s ????? %d > %d+%d\n",cbp->name,nev,cbp->nevents[icb], 
cbp->nevents[icb_next]); fflush(stdout); 
} 

#ifdef DEBUG
      printf("[%1d][%2d] GET2: nev=%d (chunk=%d nevents[%1d]=%d\n", 
			 id,i,nev,chunk,icb,cbp->nevents[icb]); fflush(stdout);
#endif
      if(cbp->nevents[icb] >= nev) /* use first buffer only - enough events */ 
      {
#ifdef DEBUG
        printf("[%1d][%2d] GET2: 1: buf %d has %d events (chunk=%d)\n", 
			   id,i,icb,cbp->nevents[icb],nev); fflush(stdout);
#endif
        /* attach buffer */ 
        cbp->nattach[icb] ++; 
        cbp->buf1[id] = icb; 
        cbp->nbuf[id] = 1;

        /* get the pointer to the first valid event in the buffer */ 
        buf =  cbp->evptr1[icb]; 
#ifdef DEBUG
        printf("[%1d][%2d] GET2: 1: first event 0x%08x from icb=%d (0x%08x)\n", 
			   id,i,cbp->evptr1[icb],icb,cbp); fflush(stdout);
#endif

        lenbuf = 0; 
        FILL_EVENT_ARRAY(0, nev, 1);
        cbp->evptr1[icb] = buf; /* new first valid event */ 

        /* if buffer is empty now switch to the next one (??? BUILDING THREAD DID NOT PROCESS IT YET !!!???) */ 
        if(cbp->nevents[icb]==0)
        {
          READ_LOCK; 
          cbp->read = (icb + 1) % QSIZE; 
          READ_SIGNAL; /* let a waiting writer know there's room */ 
          READ_UNLOCK;
        }
      }
      else /* use both current and next buffer */
      {
#ifdef DEBUG
        printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"); 
        printf("[%1d][%2d] GET2: 21: buf %d has %d events (chunk=%d)\n", 
			   id,i,icb,cbp->nevents[icb],nev); fflush(stdout);
#endif
        /* attach first buffer */ 
        cbp->nattach[icb] ++; 
        cbp->buf1[id] = icb; 
        cbp->nbuf[id] = 1; 

        /* get the pointer to the first valid event in the buffer */ 
        buf =  cbp->evptr1[icb]; 
#ifdef DEBUG
        printf("[%1d][%2d] GET2: 21: first event 0x%08x from icb=%d (0x%08x)\n", 
			   id,i,cbp->evptr1[icb],icb,cbp); fflush(stdout);
#endif
        /* get the number of events in the buffer */ 
        nev1 = cbp->nevents[icb]; 
#ifdef DEBUG
        printf("[%1d][%2d] GET2: 21: nev1=%d icb=%d (0x%08x)\n",id,i,nev1,icb,cbp); fflush(stdout);
#endif

        lenbuf = 0; 
        FILL_EVENT_ARRAY(0, nev1, 2);

        /* switch to the next buffer */ 
        READ_LOCK;
        cbp->read = (icb + 1) % QSIZE;
        READ_SIGNAL; /* let a waiting writer know there's room */ 
        READ_UNLOCK; 
      
        icb = (icb + 1) % QSIZE; 
        if(cbp->read == cbp->write) printf("[%1d][%2d] GET2: NEVER COME HERE !!!\n",id,i); 

#ifdef DEBUG
        printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"); 
        printf("[%1d][%2d] GET2: 22: buf %d has %d events (chunk=%d)\n", 
			   id,i,icb,cbp->nevents[icb],nev); fflush(stdout);
#endif
        /* attach second buffer */ 
        cbp->nattach[icb] ++; 
        cbp->nbuf[id] ++; 

        /* get the pointer to the first valid event in the buffer */ 
        buf =  cbp->evptr1[icb];
#ifdef DEBUG
        printf("[%1d][%2d] GET2: 22: first event 0x%08x from icb=%d (0x%08x)\n", 
			   id,i,cbp->evptr1[icb],icb,cbp); fflush(stdout);
#endif
 
        /* get the number of events in the buffer */ 
        nev2 = nev - nev1; 
#ifdef DEBUG
        printf("[%1d][%2d] GET2: 22: nev2=%d (%d-%d) icb=%d (0x%08x)\n",id,i,nev2,nev,nev1,icb,cbp); fflush(stdout);
#endif

if(cbp->nevents[icb] < nev2) 
{ 
printf("%s cbp->nevents[%d]=%d < nev2=%d\n",cbp->name,cbp->nevents[icb],nev2); 
 fflush(stdout); 
printf("GET2: SHOULD NEVER COME HERE !!!\n"); fflush(stdout); 
exit(0); 
} 

        FILL_EVENT_ARRAY(nev1, nev, 3);
        cbp->evptr1[icb] = buf; /* new first valid event */ 
        /* if buffer is empty now switch to the next one */ 
        if(cbp->nevents[icb]==0) 
        {
#ifdef DEBUG
		  printf("[%1d][%2d] GET2: 2: buffer is empty, switch to the next one\n",id,i); fflush(stdout); 
#endif
          READ_LOCK;
          cbp->read = (icb + 1) % QSIZE;
          READ_SIGNAL; /* let a waiting writer know there's room */ 
          READ_UNLOCK;
        }
      }
    }
    else /* do we need it ??? */ 
    {
      nevbuf = -1; 

      READ_LOCK; 
      cbp->read = (cbp->read + 1) % QSIZE; 
      READ_SIGNAL; /* let a waiting writer know there's room */ 
      READ_UNLOCK; 
      printf("[%1d][%2d] GET2: set cbp->read=%d (WHY WE ARE HERE ???\n",id,i,cbp->read); 
      printf("[%1d][%2d] GET2: return -1 ?? (cbp=0x%08x)\n",id,i,cbp); fflush(stdout); 
    }

  } /* for() over rocs */

#ifdef DEBUG
  printf("\n[%1d] GET2: =========================================================================\n\n",id);  fflush(stdout); 
#endif

  *nphys = nevphys / nrocs; /* we are summing 'nevphys' for every roc */

  return(nev);
}



/******************************************************************/
/******************************************************************/
/******************************************************************/




/*
 * get_cb_data() gets the oldest data in the circular buffer.
 * If there is none, it waits until new data appears.
 *   returns the number of events in buffer or -1 if no buffer
 */

/* USED TO FLASH INPUT STREAMS ONLY NOW !!! */

int
get_cb_data(CIRCBUF **cbh, int id, int chunk,
            unsigned int *evptr[NCHUNKMAX],int *lenbuffer, int *rocid)
{
  CIRCBUF *cbp = *cbh;
  int nevbuf, nevphys, itmp, icb_next_available;
  int i, j, icb, icb_next, nev, nev1, nev2, numev, lenev, lenbuf;
  unsigned int *buf;

  /* entry conditions check */
  if((cbh == NULL)||(*cbh == NULL)) return(-1);
  if(cbp->deleting && (NFBUF(cbp->read,cbp->write) == 0))
  {
    printf("get_cb_data(): deleting\n");
    fflush(stdout);
    return(-1);
  }


  /* release buffers kept by 'id' */
  for(j=0; j<cbp->nbuf[id]; j++)
  {
    icb = (cbp->buf1[id]+j) % QSIZE;
#ifdef DEBUG
    printf("get_cb_data(): thread[%1d] releases buf %3d\n",id,icb);
    fflush(stdout);
#endif
    cbp->nattach[icb] --;
    if(cbp->nattach[icb] < 0)
    {
      printf("get_cb_data(): ERROR: thread[%d]: cbp->nattach[%d]=%d\n",
                    id,icb,cbp->nattach[icb]); fflush(stdout);
    }

#ifdef NOALLOC
    /* mark buffer as 'free' if .. */
    if(cbp->nattach[icb]==0) /* .. nobody holds that buffer, and .. */
    {
      if(cbp->nevents[icb]==0) /* .. all events are used from that buffer */
      {
        buf = (unsigned int *)cbp->data[icb];
#ifdef DEBUG
        printf("get_cb_data(): free buffer %d (0x%08x)\n",icb,buf);
        fflush(stdout);
#endif
        buf[0] = 0; /* mark buffer as free */
      }
    }
#endif

  }
  cbp->nbuf[id] = 0;


  /* get first buffer number with valid data
  and the number of the next buffer */
  icb = cbp->read;
  icb_next = (icb + 1) % QSIZE;

  if(NFBUF(cbp->read,cbp->write) > 1) icb_next_available = 1;
  else                                icb_next_available = 0;

  /* wait while there's nothing in the buffer */
  while(cbp->nevents[icb] == 0)
  {
#ifdef DEBUG
    printf("get_cb_data(): empty, wait for %s (rocid=%d) (adr=0x%08x)\n",
                   cbp->name,cbp->rocid,cbp); fflush(stdout);
#endif
  }

  /* get ROC id */
  *rocid  = cbp->rocid;

#ifdef Linux
  if((long)cbp->data[icb] != -1) 
#else
  if((long)cbp->data[icb] > 0)
#endif
  {
    /* get the number of events will be returned; it cannot be more then
    we have total AND in 2 buffers */
    if(icb_next_available)
    {
      if(cbp->nevents[icb_next]<=0)
      {
        printf("ERRR <<<%d>>>\n",cbp->nevents[icb_next]);
        fflush(stdout);
      }
      nev=MIN(chunk,(cbp->nevents[icb]+cbp->nevents[icb_next]));
    }
    else
    {
      nev=MIN(chunk,cbp->nevents[icb]);
    }
  }
  lenbuf = 0;


  /*GET_ONE_ROC_DATA*/
  /* if buffer pointer is greater then zero then process the buffer */
  /* -1 which means no buffer !!! */
#ifdef Linux
  if((long)cbp->data[icb] != -1)
#else
  if((long)cbp->data[icb] > 0)
#endif
  {
    nevbuf = nev;
if(cbp->nevents[icb] < 0)
{
printf("%s ????? \n",cbp->name);fflush(stdout);
printf("%s ????? %d > %d+%d\n",cbp->name,nev,cbp->nevents[icb],
cbp->nevents[icb_next]); fflush(stdout);
}
    if(cbp->nevents[icb] >= nev) /* use first buffer only - enough events */
    {
      /* attach buffer */
      cbp->nattach[icb] ++;
      cbp->buf1[id] = icb;
      cbp->nbuf[id] = 1;
      /* get the pointer to the first valid event in the buffer */ \
      buf =  cbp->evptr1[icb]; \
      /* fill array of pointers to events */ \
      lenbuf = 0; \
      for(j=0; j<nev; j++) \
      { \
        evptr[j] = buf; \
        lenev = buf[0]+1; \
        lenbuf += lenev; /* returned buffer length in words */ \
        nevphys += ( (((buf[1]>>16)&0xff)==0) ? 0 : 1 ); \
        buf += lenev; \
      } \
      cbp->evptr1[icb] = buf; /* new first valid event */ \
      cbp->nevents[icb] -= nev; /* update the number of events in buffer */ \
      /* if buffer is empty now switch to the next one */ \
      if(cbp->nevents[icb]==0) \
      { \
        READ_LOCK; \
        cbp->read = (icb + 1) % QSIZE; \
        /*printf("GET3: set cbp->read=%d\n",cbp->read);*/ \
        /* let a waiting writer know there's room */ \
        READ_SIGNAL;
        READ_UNLOCK;
      } \
    } \
    else /* use both current and next buffer */ \
    { \
      /* attach first buffer */ \
      cbp->nattach[icb] ++; \
      cbp->buf1[id] = icb; \
      cbp->nbuf[id] = 1; \
      /* get the pointer to the first valid event in the buffer */ \
      buf =  cbp->evptr1[icb]; \
      /* get the number of events in the buffer */ \
      nev1 = cbp->nevents[icb]; \
      lenbuf = 0; \
      for(j=0; j<nev1; j++) \
      { \
        evptr[j] = buf; \
        lenev = buf[0]+1; \
        lenbuf += lenev; /* returned buffer length in words */ \
        nevphys += ( (((buf[1]>>16)&0xff)==0) ? 0 : 1 ); \
        buf += lenev; \
      } \
      cbp->nevents[icb] -= nev1; /* update the number of events in buffer */ \
      /* switch to the next buffer */ \
      READ_LOCK; \
      cbp->read = (icb + 1) % QSIZE; \
      /* let a waiting writer know there's room */ \
      READ_SIGNAL; \
      READ_UNLOCK; \
      /*printf("GET4: set cbp->read=%d\n",cbp->read);*/ \
      \
      icb = (icb + 1) % QSIZE; \
\
\
\
    if(cbp->read == cbp->write) printf("  GET41: NEVER COME HERE !!!\n"); \
\
\
\
      /* attach second buffer */ \
      cbp->nattach[icb] ++; \
      cbp->nbuf[id] ++; \
      /* get the pointer to the first valid event in the buffer */ \
      buf =  cbp->evptr1[icb]; \
      /* get the number of events in the buffer */ \
      nev2 = nev - nev1; \
/*printf("[%1d] get_cb_data(): nev2=%d icb=%d (0x%08x)\n",id,nev2,icb,cbp); fflush(stdout);*/ \
if(cbp->nevents[icb] < nev2) \
{ \
printf("%s cbp->nevents[%d]=%d < nev2=%d\n",cbp->name,cbp->nevents[icb],nev2); \
 fflush(stdout); \
printf("SHOULD NEVER COME HERE !!!\n"); fflush(stdout); \
exit(0); \
} \
      /* fill array of pointers to events from current buffer */ \
      for(j=nev1; j<nev; j++) \
      { \
        evptr[j] = buf; \
        lenev = buf[0]+1; \
        lenbuf += lenev; /* returned buffer length in words */ \
        nevphys += ( (((buf[1]>>16)&0xff)==0) ? 0 : 1 ); \
        buf += lenev; \
      } \
      cbp->evptr1[icb] = buf; /* new first valid event */ \
      cbp->nevents[icb] -= nev2; /* update the number of events in buffer */ \
      /* if buffer is empty now switch to the next one */ \
      if(cbp->nevents[icb]==0) \
      {
        READ_LOCK; \
        cbp->read = (icb + 1) % QSIZE; \
        /* let a waiting writer know there's room */ \
        READ_SIGNAL; \
        READ_UNLOCK; \
        /*printf("GET5: set cbp->read=%d\n",cbp->read);*/ \
      } \
    } \
  } \
  else /* do we need it ??? */ \
  { \
    nevbuf = -1; \
    READ_LOCK; \
    cbp->read = (cbp->read + 1) % QSIZE; \
    /* let a waiting writer know there's room */ \
    READ_SIGNAL; \
    READ_UNLOCK; \
    printf("GET6: set cbp->read=%d (WHY WE ARE HERE ???\n",cbp->read);
    printf("get_cb_data(): return -1 ?? (cbp=0x%08x)\n",cbp); fflush(stdout); \
  }


  
  *lenbuffer = lenbuf;

  return(nevbuf);
}


char *
get_cb_name(CIRCBUF **cbh)
{
  CIRCBUF *cbp = *cbh;

  if((cbh == NULL)||(*cbh == NULL)) return("UNKNOWN");

  if(cbp == NULL) return("UNKNOWN");
  else            return(cbp->name);
}


