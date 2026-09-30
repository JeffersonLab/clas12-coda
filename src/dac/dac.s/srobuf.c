
/* srobuf.c - library for the memory allocation system */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define logMsg printf

#define SRO_LOCK   pthread_mutex_lock(&bbp->sro_lock)
#define SRO_UNLOCK pthread_mutex_unlock(&bbp->sro_lock)

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

#include "srobuf.h"


//#define DEBUG

#define USE_COND


/* returns pool id */

SROBUF *
sro_new(int id, int nbufs, int nbytes)
{
  SROBUF *bbp;
  int i;

  /* check parameters */
  if(nbufs > NSROBUFFERS)
  {
    logMsg("sro_new: ERROR: nbufs=%d (must be <= %d\n",
      nbufs,NSROBUFFERS,3,4,5,6);
    //return(0);
    exit(1);
  }

  if(nbufs < 2)
  {
    logMsg("sro_new: ERROR: nbufs=%d (must be at least 2\n",
      nbufs,2,3,4,5,6);
    return(0);
  }

  /* allocate structure */
  bbp = (SROBUF *) malloc(sizeof(SROBUF));
  if(bbp == NULL)
  {
    logMsg("sro_new: ERROR: cannot allocate memory for 'bbp'\n",
      1,2,3,4,5,6);
    return(0);
  }
  else
  {
    bzero((void *)bbp,sizeof(SROBUF));
  }

  /* allocate buffers */
  bbp->nbufs = nbufs;
  bbp->nbytes = nbytes;
  for(i=0; i<bbp->nbufs; i++)
  {
    bbp->data[i] = (unsigned int *) malloc(bbp->nbytes);
    if(bbp->data[i] == NULL)
    {
      logMsg("sro_new: ERROR: buffer allocation FAILED\n",1,2,3,4,5,6);
      return(0);
    }
  }

  /* initialize semaphores */
  pthread_mutex_init(&bbp->sro_lock, NULL);
#ifdef USE_COND
  pthread_cond_init(&bbp->sro_cond, NULL);
#endif

  /* initialize index */
  bbp->read = 0;
  bbp->write = 1;

  /* reset cleanup condition */
  bbp->cleanup = 0;

  bbp->id = id;

  /*
  logMsg("sro_new: 'big' buffer id=%d created (addr=0x%08x, %d bufs, %d size)\n",
		 id,bbp,bbp->nbufs,bbp->nbytes,5,6);
  */
  return(bbp);
}


/* */

void
sro_delete(SROBUF **bbh)
{
  SROBUF *bbp = *bbh;
  int i;

printf("sro_delete 0: 0x%08x\n",bbh);fflush(stdout);

  if((bbh == NULL)||(*bbh == NULL)) return;

  pthread_mutex_unlock(&bbp->sro_lock);
  pthread_mutex_destroy(&bbp->sro_lock);
#ifdef USE_COND
  pthread_cond_destroy(&bbp->sro_cond);
#endif

printf("sro_delete 5\n");fflush(stdout);

  /* free buffers */
  for(i=0; i<bbp->nbufs; i++)
  {
printf("sro_delete [%d]\n",i);fflush(stdout);
    free(bbp->data[i]);
printf("sro_delete (%d)\n",i);fflush(stdout);
  }

  /* free 'bbp' structure */
printf("sro_delete 6\n");fflush(stdout);
  free(bbp);
printf("sro_delete 7\n");fflush(stdout);
}


/* */

void
sro_cleanup(SROBUF **bbh)
{
  SROBUF *bbp = *bbh;
  int i;

printf("sro_cleanup 0: 0x%08x\n",bbh);fflush(stdout);

  if((bbh == NULL)||(*bbh == NULL)) return;

printf("sro_cleanup 1: 0x%08x\n",bbp);fflush(stdout);

  bbp->cleanup = 1;
printf("sro_cleanup 2\n");fflush(stdout);

  return;
}

void
sro_init(SROBUF **bbh)
{
  SROBUF *bbp = *bbh;
  int i;

  if((bbh == NULL)||(*bbh == NULL))
  {
    logMsg("sro_init: ERROR: bbh=0x%08x *bbh=0x%08x\n",bbh,*bbh,3,4,5,6);
    return;
  }

  /* initialize index */
  bbp->read = 0;
  bbp->write = 1;

  /* reset cleanup condition */
  bbp->cleanup = 0;


  /* need that ? */
  /*printf("sro_init: clear memory for %d buffers\n",bbp->nbufs);fflush(stdout);*/
  for(i=0; i<bbp->nbufs; i++)
  {
    //printf("sro_init: buffer[%d]: clear %d bytes starting from 0x%08x\n",i,bbp->nbytes,bbp->data[i]);fflush(stdout);
    memset(bbp->data[i],0,bbp->nbytes);
  }

  return;
}










/***************************************************************/
/***************************************************************/
/***************************************************************/




/* write method: gets free buffer from the 'pool' for writing; */
/* it waits for available buffer and returns buffer pointer */

unsigned int *
sro_write(SROBUF **bbh, int *icb_out)
{
  SROBUF *bbp = *bbh;
  int icb;

  if((bbh == NULL)||(*bbh == NULL))
  {
    logMsg("[%d] sro_write ERROR 1\n",bbp->id,2,3,4,5,6); 
    return(NULL);
  }

  if(bbp->cleanup)
  {
    logMsg("[%d] sro_write 1: return(NULL) on bbp->cleanup=%d condition\n",
      bbp->id,bbp->cleanup,3,4,5,6); 
    return(NULL);
  }

  SRO_LOCK;

#ifdef DEBUG
  printf("[%d] sro_write (in):          write=%d read=%d\n",bbp->id,bbp->write,bbp->read);fflush(stdout);
#endif

  /* try to take next (empty) buffer; if not available - sleep and try again */
  icb = (bbp->write + 1) % bbp->nbufs;  

  //if(icb == bbp->read) printf("[%d] sro_write: waiting for buffer (write=%d read=%d)\n",bbp->id,bbp->write,bbp->read);
  while(icb == bbp->read)
  {
    if(bbp->cleanup)
    {
      logMsg("[%d] sro_write: return(NULL) on bbp->cleanup=%d condition\n",
        bbp->id,bbp->cleanup,3,4,5,6);
      SRO_UNLOCK;
      return(NULL);
    }

#ifdef USE_COND
    pthread_cond_wait(&bbp->sro_cond, &bbp->sro_lock);
#else
    /*printf("[%d] sro_write1: waiting for buffer (write=%d read=%d) unlock \n",bbp->id,bbp->write,bbp->read);*/
    //printf("  [%d] sro_write: waiting for icb=%d to be released by reader ...\n",bbp->id,icb);
    SRO_UNLOCK;
    usleep(1000/*0*/); /* was 10000 */
    SRO_LOCK;
    /*printf("[%d] sro_write2: waiting for buffer (write=%d read=%d) lock\n",bbp->id,bbp->write,bbp->read);*/
#endif	
  }
  //printf("  [%d] sro_write: grabbed icb=%d !\n",bbp->id,icb);fflush(stdout);

  /* set 'write' pointer to the next buffer - claim that buffer for writing */
  bbp->write = icb;
  *icb_out = icb;
#ifdef USE_COND
  pthread_cond_signal(&bbp->sro_cond);
#endif

#ifdef DEBUG
  printf("[%d] sro_write (out):         write=%d read=%d\n",bbp->id,bbp->write,bbp->read);fflush(stdout);
#endif

  SRO_UNLOCK;

  return(bbp->data[icb]);
}


unsigned int *
sro_write_current(SROBUF **bbh, int *icb_out)
{
  SROBUF *bbp = *bbh;
  int icb;

  if((bbh == NULL)||(*bbh == NULL)) return(NULL);
  
  SRO_LOCK;
  icb = bbp->write;  
  *icb_out = icb;
  SRO_UNLOCK;

  //printf("3: icb=%d, bbp->data[icb]=0x%08x\n",icb,bbp->data[icb]);

  return(bbp->data[icb]);
}












unsigned int *
sro_write_(SROBUF **bbh, int *icb_out)
{
  SROBUF *bbp = *bbh;
  int icb;

  if((bbh == NULL)||(*bbh == NULL))
  {
    logMsg("[%d] sro_write ERROR 1\n",bbp->id,2,3,4,5,6); 
    return(NULL);
  }

  if(bbp->cleanup)
  {
    logMsg("[%d] sro_write 1: return(NULL) on bbp->cleanup=%d condition\n",
      bbp->id,bbp->cleanup,3,4,5,6); 
    return(NULL);
  }

  SRO_LOCK;

#ifdef DEBUG
  printf("[%d] sro_write (in):          write=%d read=%d\n",bbp->id,bbp->write,bbp->read);fflush(stdout);
#endif

  /* try to take next (empty) buffer; if not available - sleep and try again */
  icb = (bbp->write + 1) % bbp->nbufs;  

  //if(icb == bbp->read) printf("[%d] sro_write_: waiting for buffer (write=%d read=%d)\n",bbp->id,bbp->write,bbp->read);
  while(icb == bbp->read)
  {
    if(bbp->cleanup)
    {
      logMsg("[%d] sro_write: return(NULL) on bbp->cleanup=%d condition\n",
        bbp->id,bbp->cleanup,3,4,5,6);
      SRO_UNLOCK;
      return(NULL);
    }

#ifdef USE_COND
    pthread_cond_wait(&bbp->sro_cond, &bbp->sro_lock);
#else
    /*printf("[%d] sro_write1: waiting for buffer (write=%d read=%d) unlock \n",bbp->id,bbp->write,bbp->read);*/
    //printf("  [%d] sro_write: waiting for icb=%d to be released by reader ...\n",bbp->id,icb);
    SRO_UNLOCK;
    usleep(1000/*0*/); /* was 10000 */
    SRO_LOCK;
    /*printf("[%d] sro_write2: waiting for buffer (write=%d read=%d) lock\n",bbp->id,bbp->write,bbp->read);*/
#endif	
  }
  //printf("  [%d] sro_write: grabbed icb=%d !\n",bbp->id,icb);fflush(stdout);

  /* set 'write' pointer to the next buffer - claim that buffer for writing */
  bbp->write = icb;
  *icb_out = icb;
#ifdef USE_COND
  pthread_cond_signal(&bbp->sro_cond);
#endif

#ifdef DEBUG
  printf("[%d] sro_write (out):         write=%d read=%d\n",bbp->id,bbp->write,bbp->read);fflush(stdout);
#endif

  SRO_UNLOCK;

  return(bbp->data[icb]);
}



/***************************************************************/
/***************************************************************/
/***************************************************************/



  



/*check if at least one buffer available for reading*/
int
sro_get_check(SROBUF **bbh)
{
  SROBUF *bbp = *bbh;
  int icb;

  //SRO_LOCK;
  icb = (bbp->read + 1) % bbp->nbufs; /* try to get next (full) buffer */
  //SRO_UNLOCK;

  /* check if at least one buffer released by writer */
  if(icb == bbp->write) /* if there are no buffers */
  {
    return(0);
  }
  else
  {
    return(1);
  }
}



unsigned int *
sro_get(SROBUF **bbh, int *icb_out)
{
  SROBUF *bbp = *bbh;
  int icb;

  SRO_LOCK;

  /* try to get next (full) buffer; if not available - sleep */
  icb = (bbp->read + 1) % bbp->nbufs;
  //printf("icb=%d get1\n",icb);fflush(stdout);

  /* wait until buffer will be released by writer */
  while(icb == bbp->write) /* if there are no buffers - wait */
  {
#ifdef USE_COND
    pthread_cond_wait(&bbp->sro_cond, &bbp->sro_lock);
#else
    //printf("    [%d] sro_get: waiting for icb=%d to be released by writer ...\n",bbp->id,icb);fflush(stdout);
    SRO_UNLOCK;
    usleep(100/*00*/);
    SRO_LOCK;
#endif
  }
  //printf("    [%d] sro_get: grabbed icb=%d !\n",bbp->id,icb);fflush(stdout);

  /* set 'read' pointer to the next buffer - claim that buffer for reading */
  bbp->read = icb;
  *icb_out = icb;
  //printf("icb=%d get3\n\n",icb);fflush(stdout);
#ifdef USE_COND
  pthread_cond_signal(&bbp->sro_cond);
#endif

  SRO_UNLOCK;

  return(bbp->data[icb]);
}






unsigned int *
sro_get1(SROBUF **bbh, int *icb_out)
{
  SROBUF *bbp = *bbh;
  int icb;

  SRO_LOCK;


  /* try to get next (full) buffer; if not available - sleep */
  icb = (bbp->read + 1) % bbp->nbufs;
  //printf("icb=%d get1\n",icb);fflush(stdout);


  /* wait until buffer will be released by writer */
  while(icb == bbp->write) /* if there are no buffers - wait */
  {
#ifdef USE_COND
    pthread_cond_wait(&bbp->sro_cond, &bbp->sro_lock);
#else
    //printf("    [%d] sro_get1: waiting for icb=%d to be released by writer ...\n",bbp->id,icb);fflush(stdout);
    SRO_UNLOCK;
    usleep(10000);
    SRO_LOCK;
#endif
  }
  //printf("    [%d] sro_get1: grabbed icb=%d !\n",bbp->id,icb);fflush(stdout);

  /* set 'read' pointer to the next buffer - claim that buffer for reading */
  bbp->read = icb;
  *icb_out = icb;
  //printf("icb=%d get3\n\n",icb);fflush(stdout);
#ifdef USE_COND
  pthread_cond_signal(&bbp->sro_cond);
#endif

  SRO_UNLOCK;

  return(bbp->data[icb]);
}




