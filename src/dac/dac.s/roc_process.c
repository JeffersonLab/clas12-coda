
/* roc_process.c - ROC processing service routines; can be run at the same
   CPU as coda_roc or on secondary CPU */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <dlfcn.h>
#include <sys/mman.h>

#define MYCLOCK NANOMICRO

#ifdef Linux
#include <sys/prctl.h>
#endif

#include "da.h"

#include "circbuf.h"
#include "bigbuf.h"



#undef DEBUG



/***************************************************************************/
/******************************** user code ********************************/

/*
#include "coda.h"
#include "tt.h"
*/

#include "pmc.h"

#ifdef __cplusplus
typedef void 		(*VOIDFUNCPTR) (...); /* ptr to function returning void */
#else
typedef void 		(*VOIDFUNCPTR) (); /* ptr to function returning void */
#endif			/* _cplusplus */

#include "rolInt.h"
static ROLPARAMS rolP2;


/*extern TTSPtr ttp;*/ /* pointer to TTS structures for current roc */

/* copy 'bufin' to 'bufout' */
/* input: bufout[0] - max length of output event */

int
proc_copy(int *bufin, int *bufout, int pid)
{
  int lenin;

  lenin = bufin[0] + 1; /* bufin[0] does not include itself */
  memcpy(bufout, bufin, (lenin<<2));
  /*
  printf("proc_copy: copy event from 0x%08x to 0x%08x, lenw=%d\n",
    bufin,bufout,lenin);
  */

  return(lenin);
}


int
proc_tt(int *bufin, int *bufout, int pid, int *nev)
{
  ROLPARAMS *rolP = &rolP2;
  int len, ii;

  /* set input/output pointers and cleanup first
  word in output (will be CODA fragment length) */
  rolP->dabufpi = (int32_t *) bufin;
  rolP->dabufp = (int32_t *) bufout;
  bufout[0] = 0;

#ifdef DEBUG
  printf("proc_tt reached\n");fflush(stdout);
#endif

  /*call ROL2 trigger routine */
  /*DA_POLL_PROC==__poll()==CDOPOLL==cdopolldispatch()*/
  rolP->daproc = DA_POLL_PROC;
  (*rolP->rol_code) (rolP);

  /* let see now what we've got from ROL2 */
  len = rolP->user_storage[0];
  *nev = rolP->user_storage[1];

  if(len > NWBOS)
  {
    printf("proc_tt ERROR: on output event length=%d (words) is too big\n",len);
    return(0);
  }
  else if(len < 1)
  {
    printf("proc_tt: ERROR: on output len = %d\n",len);
    return(0);
  }

#ifdef DEBUG
  printf("proc_tt: return len=%d, nev=%d\n",len,*nev);fflush(stdout);
#endif

  return(len);
}


/**********************/
/* STANDARD FUNCTIONS */

int
proc_download(char *rolname, char *rolparams, int pid)
{
  ROLPARAMS *rolP = &rolP2;
  int res;

  /*******************************************/
  /* downloading rol2 we suppose to run here */

  res = codaLoadROL(rolP,rolname,rolparams);
  /*
          "/usr/clas/devel/coda/src/rol/rol/VXWORKS_ppc/obj/rol2_tt.o",
          "usr");
  */
  if(res)
  {
    return(-1);
  }

  rolP->daproc = DA_INIT_PROC;
  rolP->pid = pid;

  /* execute ROL init procedure (described in 'rol.h') */
  (*(rolP->rol_code)) (rolP);

  /* check if initialization was successful */
  if(rolP->inited != 1)
  {
    printf ("ERROR: ROL initialization failed\n");
    return(-2);
  }

  rolP->daproc = DA_DOWNLOAD_PROC;
  (*rolP->rol_code) (rolP);

  return(0);
}

int
proc_prestart(int pid)
{
  ROLPARAMS *rolP = &rolP2;
  /*
  rolP->runNumber = object->runNumber;
  rolP->runType = object->runType;
  rolP->recNb = 0;
  */
  rolP->daproc = DA_PRESTART_PROC;
  (*rolP->rol_code) (rolP);

  return(0);
}

int
proc_go(int pid)
{
  ROLPARAMS *rolP = &rolP2;

  printf("proc_go reached\n");fflush(stdout);

  rolP->daproc = DA_GO_PROC;
  (*rolP->rol_code) (rolP);

  printf("proc_go done\n");fflush(stdout);
  return(0);
}

int
proc_end()
{
  ROLPARAMS *rolP = &rolP2;

  printf("proc_end reached\n");fflush(stdout);

  rolP->daproc = DA_END_PROC;
  (*rolP->rol_code) (rolP);

  printf("proc_end done\n");fflush(stdout);
  return(0);
}

int
proc_poll(int *bufin, int *bufout, int pid, int *nev)
{
  int len;

  if(pid == -1)
  {
    len = proc_copy(bufin, bufout, pid);
    *nev = 1;
  }
  else
  {
    len = proc_tt(bufin, bufout, pid, nev);
  }

  return(len);
}



/***************************************************************************/
/******************************* proc_thread *******************************/

#define PROC_TIMER_START timeout = time(0) + token_interval/60

/* special events must be in separate buffer, buffer number must be -1 */
/* from BIGNET structure it uses following:
  BIGBUF *gbigin - input data buffer
  BIGBUF *gbigout - output data buffer
  token_interval - for time profiling
  failure - set it to 0 just in case
  doclose - just for printing
 */
void 
proc_thread(BIGNET *bigprocptrin)
{
  unsigned int offsetin = 0;
  ROLPARAMS *rolP = &rolP2;
  static int length, status, ifend;
  int i, jj, llen, llenw, evsz, res, nevent, iev, lenin, lenout, pid, nbuffer;
  unsigned int maxoutbuflen;
  unsigned int *bigbufin, *bigbufout, *bufin, *bufout;
  unsigned int timeout;
  int nev_per_block;
  static BIGNET *bigprocptr;
  static unsigned int offset;
  static int token_interval;

/* timing */
  hrtime_t start, end, time1, time2, icycle, cycle = 100;
  static hrtime_t sum;

#ifdef Linux
  prctl(PR_SET_NAME,"coda_proc");
#endif

  /*printf("input: bigprocptrin=0x%08x offsetin=0x%08x\n",
    bigprocptrin,offsetin);*/
  bigprocptr = bigprocptrin;
  offset = offsetin;
  token_interval = bigprocptr->token_interval;

  printf("proc_thread reached\n");fflush(stdout);
  printf("proc_thread reached\n");fflush(stdout);
  printf("proc_thread reached\n");fflush(stdout);
  printf("proc_thread reached\n");fflush(stdout);
  printf("proc_thread reached\n");fflush(stdout);
  printf("proc_thread reached\n");fflush(stdout);
  sleep(1);
  printf("proc_thread reached\n");fflush(stdout);
  printf("proc_thread reached\n");fflush(stdout);
  printf("proc_thread reached\n");fflush(stdout);
  printf("proc_thread reached\n");fflush(stdout);
  printf("proc_thread reached\n");fflush(stdout);
  printf("proc_thread reached\n");fflush(stdout);

  nevent = icycle = time1 = time2 = 0;
  timeout = 0;
  bigbufout = NULL;
  nbuffer = 1;

  /* get output buffer maximum length in bytes */
  maxoutbuflen = bigprocptr->gbigout->nbytes;
  /*printf("proc_thread: maxoutbuflen=%u bytes\n",maxoutbuflen);*/

  do
  {
    icycle ++;
    start = gethrtime();
#ifdef DEBUG
    printf("proc: next itteration, icycle=%d\n",icycle);fflush(stdout);
#endif

    /* wait for input buffer */
    bigbufin = bb_read(&(bigprocptr->gbigin));
    if(bigbufin == NULL)
    {
      printf("proc_thread: ERROR: bigbufin==NULL\n");fflush(stdout);
      break;
    }

    end = gethrtime();
    time1 += (end-start)/MYCLOCK;

    start = gethrtime();

    bigprocptr->failure = 0;

    /* remember some values (do not need all of them ..) */
    llenw = bigbufin[BBIWORDS];
    llen = bigbufin[BBIWORDS] << 2;
	/*printf("proc_thread: llen=%d\n",llen);*/
    nevent += bigbufin[BBIEVENTS];
    ifend = bigbufin[BBIEND];
    pid = bigbufin[BBIROCID];

    bufin = bigbufin + BBHEAD;

#ifdef DEBUG
    printf("proc: got input buffer 0x%08x: num=%d nev=%d lenw=%d ifend=%d pid=%d\n",
      bigbufin,bigbufin[BBIBUFNUM],bigbufin[BBIEVENTS],llenw,ifend,pid);
#endif

    /* loop over input buffer */
    for(iev=0; iev<bigbufin[BBIEVENTS]; iev++)
    {

      /* if input buffer contains special event, release output buffer if it exist */
      if((bigbufin[BBIBUFNUM] == -1) && (bigbufout != NULL))
      {
#ifdef DEBUG
        printf("proc: releasing output buffer - 0 0x%08x: num=%d nev=%d lenw=%d ifend=%d pid=%d\n",
          bigbufout,bigbufout[BBIBUFNUM],bigbufout[BBIEVENTS],bigbufout[BBIWORDS],
          bigbufout[BBIEND],bigbufout[BBIROCID]);
#endif
/*
printf("=0=> %d\n",bigbufout[BBIFD]);
*/
        bigbufout = bb_write(&(bigprocptr->gbigout));
        if(bigbufout == NULL)
        {
          printf("proc bb_write ERROR 0\n");
          break;
        }
        else /* cleanup pointer so new buffer will be obtained on next loop */
        {
          bigbufout = NULL;
        }
      }

      /* get new output buffer if needed */
      if(bigbufout == NULL)
      {
        bigbufout = bb_write_current(&(bigprocptr->gbigout));
        if(bigbufout == NULL)
        {
          printf("proc bb_write_current ERROR 1\n");
          break;
        }
        else /* initialize output buffer header */
        {
          bigbufout[BBIWORDS]  = BBHEAD;
          if(bigbufin[BBIBUFNUM] == -1)
          {
            bigbufout[BBIBUFNUM] = -1;
          }
          else
          {
            bigbufout[BBIBUFNUM] = nbuffer ++;
          }
          bigbufout[BBIROCID]  = bigbufin[BBIROCID];
          bigbufout[BBIEVENTS] = 0;
          bigbufout[BBIFD]     = bigbufin[BBIFD];
/*
printf("<--- 0x%08x 0x%08x - fd=%d\n",bigprocptr->gbigout,bigbufout,bigbufin[BBIFD]);
*/
          bigbufout[BBIEND]    = bigbufin[BBIEND];
          bufout = bigbufout + BBHEAD;
          PROC_TIMER_START;
#ifdef DEBUG
          printf("proc: got output buffer 0x%08x: num=%d nev=%d lenw=%d ifend=%d pid=%d\n",
            bigbufout,bigbufout[BBIBUFNUM],bigbufout[BBIEVENTS],bigbufout[BBIWORDS],
            bigbufout[BBIEND],bigbufout[BBIROCID]);
#endif
        }
      }


      /* process one event */
      lenin = bufin[0] + 1; /* input event length */

      /* force data coping instead of translation for special events */
      if(bigbufin[BBIBUFNUM] == -1)
      {
        printf(">>>>>>>>>>>>>>>> use pid=-1 <<<<<<<<<<<<<<<<<\n");
        lenout = proc_poll(bufin, bufout, -1, &nev_per_block);
/*printf("111 nev_per_block=%d\n",nev_per_block);*/
	  }
      else
      {
#ifdef DEBUG
        printf(">>>>>>>>>>>>>>>> use pid=%d <<<<<<<<<<<<<<<<<\n",pid);fflush(stdout);
#endif
        lenout = proc_poll(bufin, bufout, pid, &nev_per_block);
		/*printf("222 nev_per_block=%d\n",nev_per_block);*/
      }
      bufout += lenout;

      /* update output buffer header */
/*printf("333 nev_per_block=%d\n",nev_per_block);*/
      bigbufout[BBIEVENTS] += nev_per_block;
      bigbufout[BBIWORDS] += lenout;

#ifdef DEBUG
      printf("proc: after proc_poll lenout=%d\n",lenout);fflush(stdout);
      printf("proc: time(0)=%u timeout=%u\n",time(0),timeout);fflush(stdout);
      printf("proc: bufout=%u bigbufout=%u BBHEAD_BYTES=%d\n",bufout,bigbufout,BBHEAD_BYTES);fflush(stdout);
#endif


      /* release output buffer on full condition (use actual big buffer size, not SEND_BUF_SIZE !!!)
         or on timer (Sergey: not on timer, will release on input buffer processed below) */
      if( ((bigbufout[BBIWORDS]<<2) > (maxoutbuflen - SEND_BUF_MARGIN))  ||
          ((time(0) > timeout) && ((bufout - bigbufout) > BBHEAD_BYTES)) )
      {
		/*		
printf("=== PROC1: %d %d %d %d 0x%08x\n",bigbufout[0],bigbufout[1],bigbufout[2],bigbufout[3],bigbufout[4]);
		*/		

        bigbufout = bb_write(&(bigprocptr->gbigout));
        if(bigbufout == NULL)
        {
          printf("proc bb_write ERROR 2\n");
          break;
        }
        else /* cleanup pointer so new buffer will be obtained on next loop */
        {
          bigbufout = NULL;
        }
      }

      bufin += lenin;

    } /* finish processing input buffer*/




    /* always release output buffer if input buffer number equal to -1 (special events) */

    /* SERGEY: ALWAYS release output buffer after it processed !!!??? */
    if(/*(bigbufin[BBIBUFNUM] == -1) &&*/ (bigbufout != NULL))
    {
#ifdef DEBUG
    printf("proc: releasing output buffer - 2 0x%08x: num=%d nev=%d lenw=%d ifend=%d pid=%d\n",
      bigbufout,bigbufout[BBIBUFNUM],bigbufout[BBIEVENTS],bigbufout[BBIWORDS],
      bigbufout[BBIEND],bigbufout[BBIROCID]);
#endif
	/*
printf("=== PROC2: %d %d %d %d 0x%08x\n",bigbufout[0],bigbufout[1],bigbufout[2],bigbufout[3],bigbufout[4]);
	*/
      bigbufout = bb_write(&(bigprocptr->gbigout));
      if(bigbufout == NULL)
      {
        printf("proc bb_write ERROR 3\n");
        break;
      }
      else /* cleanup pointer so new buffer will be obtained on next loop */
      {
        bigbufout = NULL;
      }
	}

/*timing */
    end = gethrtime();
    time2 += (end-start)/MYCLOCK;
    if(nevent != 0 && icycle >= cycle)
    {
#if 1
      printf("proc_thread: waiting=%7llu processing=%7llu microsec per event (nev=%d)\n",
	    time1/nevent,time2/nevent,nevent/icycle);
#endif
      nevent = icycle = time1 = time2 = 0;
    }

    if(ifend == 1)
    {
      printf("proc_thread: ifend==1 (%d), ending ..\n",bigprocptr->doclose);fflush(stdout);
      sleep(1);
	}

	/* ????? why 'bigprocptr->doclose == 1' ??? */
    /* exit the loop if 'End' condition was received */
    if(ifend == 1/* && bigprocptr->doclose == 1*/)
    {
      printf("proc_thread: END condition received\n");fflush(stdout);
      for(jj=0; jj<1/*0*/; jj++) {printf("PROC_THREAD GOT END %d SEC\n",jj);sleep(1);}
      break;
    }

  } while(1);

printf("roc_process cleanup +++++++++++++++++++++++++++++++++++++ 1\n");fflush(stdout);
sleep(1);

  /* force input 'big' buffer read/write methods to exit */
  bb_cleanup(&(bigprocptr->gbigin));


  /*sergey 2016 */
  printf("roc_process: calls 'proc_end'\n");
  proc_end();
  /*sergey 2016 */


  printf("PROC THREAD EXIT\n");
}
