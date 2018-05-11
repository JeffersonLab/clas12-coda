
/* largebuf.c - library for event recorder; assumes one thread(et_reader) calling lb_write,
 and 'nbufs' threads(evio_writers) calling lb_read; every evio_writer thread used its own buffer */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/********** LARGE BUFFERS MANAGEMENT PACKAGE ********************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#include "largebuf.h"

/*
#define DEBUG
*/

/* returns pool id */

LARGEBUF *
lb_new(int id, int nbufs, int nbytes)
{
  LARGEBUF *lbp;
  int i;

  /* check parameters */
  if(nbufs > NBUFS)
  {
    printf("lb_new: ERROR: nbufs=%d (must be <= %d\n",nbufs,NBUFS);
    return(0);
  }

  /* allocate structure */
  lbp = (LARGEBUF *) malloc(sizeof(LARGEBUF));
  if(lbp == NULL)
  {
    printf("lb_new: ERROR: cannot allocate memory for 'lbp'\n");
    return(0);
  }
  else
  {
    bzero((void *)lbp,sizeof(LARGEBUF));
  }

  /* allocate buffers */
  lbp->nbufs = nbufs;
  lbp->nbytes = nbytes;
  for(i=0; i<lbp->nbufs; i++)
  {
    lbp->data[i] = (unsigned int *) malloc(lbp->nbytes);
    if(lbp->data[i] == NULL)
    {
      printf("lb_new: ERROR: buffer allocation FAILED\n");
      return(0);
    }

    /* initialize semaphores */
    pthread_mutex_init(&lbp->buf_lock[i], NULL);
    pthread_cond_init(&lbp->buf_cond[i], NULL);
  }

  /* reset cleanup condition */
  lbp->cleanup = 0;

  printf("lb_new: 'big' buffer id=%d created (addr=0x%08x, %d bufs, %d size)\n",
		 id,lbp,lbp->nbufs,lbp->nbytes);

  return(lbp);
}



/* */

void
lb_delete(LARGEBUF **lbh)
{
  LARGEBUF *lbp = *lbh;
  int i;

printf("lb_delete 0: 0x%08x\n",lbh);fflush(stdout);

  if((lbh == NULL)||(*lbh == NULL)) return;

  for(i=0; i<lbp->nbufs; i++)
  {
    pthread_cond_broadcast(&lbp->buf_cond[i]);
    BUF_UNLOCK(i);
    pthread_mutex_destroy(&lbp->buf_lock[i]);

    pthread_cond_destroy(&lbp->buf_cond[i]);
  }

printf("lb_delete 5\n");fflush(stdout);

  /* free buffers */
  for(i=0; i<lbp->nbufs; i++)
  {
printf("lb_delete [%d]\n",i);fflush(stdout);
    free(lbp->data[i]);
printf("lb_delete (%d)\n",i);fflush(stdout);
  }

  /* free 'lbp' structure */
printf("lb_delete 6\n");fflush(stdout);
  free(lbp);
printf("lb_delete 7\n");fflush(stdout);
}


/* */

void
lb_cleanup(LARGEBUF **lbh)
{
  LARGEBUF *lbp = *lbh;
  int i;

printf("lb_cleanup 0: 0x%08x\n",lbh);fflush(stdout);

  if((lbh == NULL)||(*lbh == NULL)) return;

printf("lb_cleanup 1: 0x%08x\n",lbp);fflush(stdout);

  lbp->cleanup = 1;
printf("lb_cleanup 2\n");fflush(stdout);

  return;
}

void
lb_init(LARGEBUF **lbh)
{
  LARGEBUF *lbp = *lbh;
  int i;

  if((lbh == NULL)||(*lbh == NULL))
  {
    printf("lb_init: ERROR: lbh=0x%08x *lbh=0x%08x\n",lbh,*lbh);
    return;
  }

  /* reset cleanup condition */
  lbp->cleanup = 0;

  /* need that ? */
  printf("lb_init: clear memory for %d buffers\n",lbp->nbufs);fflush(stdout);
  for(i=0; i<lbp->nbufs; i++)
  {
    printf("lb_init: buffer[%d]: clear %d bytes starting from 0x%08x\n",
		   i,lbp->nbytes,lbp->data[i]);fflush(stdout);
    memset(lbp->data[i],0,lbp->nbytes);
    lbp->state[i] = BUF_EMPTY; /* mark buffer as empty */
    lbp->data[i][IBUFIND] = i;
    printf("lb_init: buffer[%d]: set buffer index to %d\n",lbp->data[i][IBUFIND]);
  }

  return;
}






/*******************************************************************************/
/*******************************************************************************/



/* write method: wait for available buffer, grab it and returns buffer pointer, NULL if cleanup, <0 if error */

unsigned int *
lb_write_grab(LARGEBUF **lbh)
{
  LARGEBUF *lbp = *lbh;
  int i, icb;

  if((lbh == NULL)||(*lbh == NULL))
  {
    printf("lb_write_grab ERROR 1\n"); 
    return(-1);
  }

  if(lbp->cleanup)
  {
    printf("lb_write_grab: return(NULL) on lbp->cleanup=%d condition\n",lbp->cleanup); 
    return(NULL);
  }


  /**********************************************/
  /* wait for reader(s) to release buffer 'icb' */

  icb = -1;
  GLOBAL_LOCK;
  while(icb < 0)
  {
    for(i=0; i<lbp->nbufs; i++)
    {
      /* if buffer empty, remember icb and break from loop */
      if((lbp->state[i] == BUF_EMPTY))
      {
        icb = i;
#ifdef DEBUG
        printf("lb_write_grab: GRAB'ing buffer %d, state=%d(EMPTY)\n",icb,lbp->state[icb]);fflush(stdout);
#endif
        break;
      }
      else
	  {
#ifdef DEBUG
        printf("lb_write_grab: SKIP'ing buffer %d, state=%d\n",i,lbp->state[i]);fflush(stdout);
#endif
	  }
    }
    if(icb<0)
	{
#ifdef DEBUG
      printf("lb_write_grab: GLOBAL_WAIT'ing for EMPTY\n");fflush(stdout);
#endif
      GLOBAL_WAIT;
	}
  }
  GLOBAL_UNLOCK;


  /*********************/
  /* grab buffer 'icb' */

  BUF_LOCK(icb);
  lbp->icb = icb;
  lbp->state[icb] = BUF_WRITING;
#ifdef DEBUG
  printf("[%d] lb_write_grab: BUF_LOCK'ed, state=%d(WRITING)\n",icb,lbp->state[icb]);fflush(stdout);
#endif
  BUF_UNLOCK(icb);

  return(lbp->data[icb]);
}


unsigned int *
lb_write_release(LARGEBUF **lbh)
{
  LARGEBUF *lbp = *lbh;
  int i, icb;

  if((lbh == NULL)||(*lbh == NULL))
  {
    printf("lb_write_release ERROR 1\n");
    return(-1);
  }

  if(lbp->cleanup)
  {
    printf("lb_write_release: return(NULL) on lbp->cleanup=%d condition\n",lbp->cleanup); 
    return(NULL);
  }


  /**********************************/
  /* release previously used buffer */

  GLOBAL_LOCK;
  icb = lbp->icb;
  lbp->state[icb] = BUF_FULL;
#ifdef DEBUG
  printf("[%d] lb_write_release: BUF_UNLOCK'ed, SET state=%d(FULL), SEND BUF_SIGNAL to [%d]\n",icb,lbp->state[icb], icb);fflush(stdout);
#endif
  BUF_SIGNAL(icb);
  GLOBAL_UNLOCK;

  return(lbp->data[icb]);
}



/* read method: grab buffer 'icb' for reading, mark as 'BUF_READING', returns buffer pointer, NULL if cleanup, <0 if error */

unsigned int *
lb_read_grab(LARGEBUF **lbh, int icb)
{
  LARGEBUF *lbp = *lbh;
  int j;

  if((lbh == NULL)||(*lbh == NULL))
  {
    printf("[%d] lb_read_grab ERROR 1\n",icb); 
    return(-1);
  }

  if((icb<0)||(icb>=lbp->nbufs))
  {
    printf("[%d] lb_read_grab ERROR 2\n",icb); 
    return(-2);
  }

  if(lbp->cleanup)
  {
    printf("[%d] lb_read_grab: return(NULL) on lbp->cleanup=%d condition\n",icb,lbp->cleanup); 
    return(NULL);
  }


  /**********************************************/
  /* wait for writer(s) to release buffer 'icb' */

  BUF_LOCK(icb);
#ifdef DEBUG
  for(j=0; j<8*icb+8; j++) printf(" ");
  printf("[%d] lb_read_grab: state=%d - BUF_LOCK'ed\n",icb,lbp->state[icb]);fflush(stdout);
#endif

  /* if buffer not full, wait */
  while((lbp->state[icb] != BUF_FULL))
  {
    if(lbp->cleanup)
    {
      printf("[%d] lb_read_grab: return(NULL) on lbp->cleanup=%d condition\n",icb,lbp->cleanup); 
      BUF_UNLOCK(icb);
      return(NULL);
	}
#ifdef DEBUG
    for(j=0; j<8*icb+8; j++) printf(" ");
    printf("[%d] lb_read_grab: state=%d - BUF_WAIT'ing for FULL\n",icb,lbp->state[icb]);fflush(stdout);
#endif
    BUF_WAIT(icb);
  }


  /*********************/
  /* grab buffer 'icb' */

  lbp->state[icb] = BUF_READING;
#ifdef DEBUG
  for(j=0; j<8*icb+8; j++) printf(" ");
  printf("[%d] lb_read_grab: GRAB'ed, state=%d(READING)\n",icb,lbp->state[icb]);fflush(stdout);
#endif

  return(lbp->data[icb]);
}



/* read method: grab buffer 'icb' for reading; returns buffer pointer, NULL if cleanup, <0 if error */

unsigned int *
lb_read_release(LARGEBUF **lbh, int icb)
{
  LARGEBUF *lbp = *lbh;
  int j;

  if((lbh == NULL)||(*lbh == NULL))
  {
    printf("[%d] lb_read_release ERROR 1\n",icb); 
    return(-1);
  }

  if((icb<0)||(icb>=lbp->nbufs))
  {
    printf("[%d] lb_read_release ERROR 2\n",icb); 
    return(-2);
  }

  if(lbp->cleanup)
  {
    printf("[%d] lb_read_release: return(NULL) on lbp->cleanup=%d condition\n",icb,lbp->cleanup); 
    return(NULL);
  }


  /******************/
  /* release buffer */

  lbp->state[icb] = BUF_EMPTY;
  GLOBAL_SIGNAL;
  BUF_UNLOCK(icb);
#ifdef DEBUG
  for(j=0; j<8*icb+8; j++) printf(" ");
  printf("[%d] lb_read_release: RELEASE'ed, state=%d(EMPTY)\n",icb,lbp->state[icb]);fflush(stdout);
#endif

  return(lbp->data[icb]);
}



