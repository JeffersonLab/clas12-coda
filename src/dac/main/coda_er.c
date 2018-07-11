
/* coda_er.c - CODA event recorder */


/* take data from ET, do not send 
#define DO_NOT_OUTPUT
*/

/* take data from ET, send to writing thread(s), do not write 
#define DO_NOT_WRITE
*/


#if defined(Linux_armv7l)

void
coda_er()
{
  printf("coda_er is dummy for ARM etc\n");
}

#else


/*---------------------------------------------------------------------------*
 *  Copyright (c) 1991, 1992  Southeastern Universities Research Association,*
 *                            Continuous Electron Beam Accelerator Facility  *
 *                                                                           *
 *    This software was developed under a United States Government license   *
 *    described in the NOTICE file included as part of this distribution.    *
 *                                                                           *
 * CEBAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606*
 *      heyes@cebaf.gov   Tel: (804) 249-7030    Fax: (804) 249-7363         *
 *---------------------------------------------------------------------------*
 * Discription: follows this header.
 *
 * Author:
 *	Graham Heyes
 *	CEBAF Data Acquisition Group
 *
 *----------------------------------------------------------------------------*/

/* INCLUDES */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>

#ifdef Linux
#include <sys/prctl.h>
#endif
#if defined __sun||LINUX
#include <dlfcn.h>
#endif
#ifdef Linux
#include <unistd.h> /* for usleep() */
#endif

#define ABS(x)      ((x) < 0 ? -(x) : (x))

#define TIMERL_VAR \
  static hrtime_t startTim, stopTim, dTim; \
  static int nTim; \
  static hrtime_t Tim, rmsTim, minTim=10000000, maxTim, normTim=1

#define TIMERL_START \
{ \
  startTim = gethrtime(); \
}

#define TIMERL_STOP(whentoprint_macros,id_macros) \
{ \
  stopTim = gethrtime(); \
  if(stopTim > startTim) \
  { \
    nTim ++; \
    dTim = stopTim - startTim; \
    /*if(histid_macros >= 0)   \
    { \
      uthfill(histi, histid_macros, (int)(dTim/normTim), 0, 1); \
    }*/														\
    Tim += dTim; \
    rmsTim += dTim*dTim; \
    minTim = minTim < dTim ? minTim : dTim; \
    maxTim = maxTim > dTim ? maxTim : dTim; \
    /*logMsg("good: %d %ud %ud -> %d\n",nTim,startTim,stopTim,Tim,5,6);*/ \
    if(nTim == whentoprint_macros) \
    { \
      printf("timer[%d]: %7llu microsec (min=%7llu max=%7llu rms**2=%7llu)\n", id_macros, \
                Tim/nTim/normTim,minTim/normTim,maxTim/normTim, \
                ABS(rmsTim/nTim-Tim*Tim/nTim/nTim)/normTim/normTim);	\
      nTim = Tim = 0; \
    } \
  } \
  else \
  { \
    /*logMsg("bad:  %d %ud %ud -> %d\n",nTim,startTim,stopTim,Tim,5,6);*/ \
  } \
}







#include "rc.h"
#include "da.h"
#include "libdb.h"

#ifdef RESTORE_OLD_SPEC_EVENT_CODING
#define EV_ER_PRESTART (EV_PRESTART-0x80)
#define EV_ER_END (EV_END-0x80)
#else
#define EV_ER_PRESTART EV_PRESTART
#define EV_ER_END EV_END
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MAXTHREAD 4

typedef int (*IFUNCPTR) ();

typedef struct ERpriv *ERp;
typedef struct ERpriv
{
  int  record_length;
  /*int*/long  split;
  char filename[128];
  pthread_t write_thread;
  pthread_t evio_thread[MAXTHREAD];
  objClass object;
  int splitnb;
  int nevents;
  /*long*/int64_t nlongs;
  int nerrors;
  int nend;

} ER_priv;


static int PrestartCount = 0;
/*static*/extern objClass localobject;
extern char configname[128]; /* coda_component.c */
extern char *mysql_host; /* coda_component.c */
extern char *expid; /* coda_component.c */
extern char *session; /* coda_component.c */

static int mbytes_in_current_run;
static int nevents_in_current_run;

#define ER_ERROR 1
#define ER_OK 0

int listSplit1(char *list, int flag, int *argc, char argv[LISTARGV1][LISTARGV2]);

/* Define time limit for staying in write loop (seconds) */
#define ER_WRITE_LOOP_TIMEOUT   10


/****************************************************************************/
/***************************** tcpServer functions **************************/

static int tcpState = DA_UNKNOWN;

void
rocStatus()
{
  /*
  printf("%d \n",tcpState);
  */
  switch(tcpState)
  {
    case DA_UNKNOWN:
      printf("unknown\n");
      break;
    case DA_BOOTING:
      printf("booting\n");
      break;
    case DA_BOOTED:
      printf("booted\n");
      break;
    case DA_CONFIGURING:
      printf("initing\n");
      break;
    case DA_CONFIGURED:
      printf("initied\n");
      break;
    case DA_DOWNLOADING:
      printf("loading\n");
      break;
    case DA_DOWNLOADED:
      printf("loaded\n");
      break;
    case DA_PRESTARTING:
      printf("prestarting\n");
      break; 
    case DA_PAUSED:
      printf("paused\n");
      break;
    case DA_PAUSING:
      printf("pausing\n");
      break;
    case DA_ACTIVATING:
      printf("activating\n");
      break;
    case DA_ACTIVE:
      printf("active\n");
      break;
    case DA_ENDING:
      printf("ending\n");
      break;
    case  DA_VERIFYING:
      printf("verifying\n");
      break;
    case DA_VERIFIED:
      printf("verified\n");
      break;
    case DA_TERMINATING:
      printf("terminating\n");
      break;
    case DA_PRESTARTED:
      printf("prestarted\n");
      break;
    case DA_RESUMING:
      printf("resuming\n");
      break;
    case DA_STATES:
      printf("states\n");
      break;
    default:
      printf("unknown\n");
  }
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/




/* put some statistic into *_option table of the database: nfile, nevent, ndata */

#define NDBVALS 5

void
update_database(ERp erp)
{
  objClass object = localobject;
  char tmpp[1000];
  MYSQL *dbsocket;
  MYSQL_RES *result;
  MYSQL_ROW row;
  time_t rawtime;


return;
printf("\n=== nlongs=%lld\n",erp->nlongs);


  mbytes_in_current_run += (erp->nlongs*4)/1000000;
  nevents_in_current_run += erp->nevents;
  time(&rawtime);

  /* connect to database */
  dbsocket = dbConnect(mysql_host, expid);
  if(dbsocket==NULL)
  {
    printf("WARN: cannot connect to the database to update run statistics in _option table\n");
  }
  else
  {
    int iii, numRows;
    char *db_field[NDBVALS] = {"nfile","nevent","ndata","unixtime","runnum"};
    char db_value[NDBVALS][80];

    sprintf(db_value[0],"%d",erp->splitnb+1);
	sprintf(db_value[1],"%d",nevents_in_current_run);
	sprintf(db_value[2],"%d",mbytes_in_current_run);
	sprintf(db_value[3],"%d",rawtime);
	sprintf(db_value[4],"%d",object->runNumber);

    for(iii=0; iii<NDBVALS; iii++)
	{
      sprintf(tmpp,"SELECT name,value FROM %s_option WHERE name='%s'",configname,db_field[iii]);
      if(mysql_query(dbsocket, tmpp) != 0)
      {
        printf("ERROR in mysql_query\n");
      }
      else
      {
        if( !(result = mysql_store_result(dbsocket)) )
        {
          printf("ERROR in mysqlStoreResult()\n");
          return;
        }
        else
        {
          numRows = mysql_num_rows(result);
          printf("nrow=%d\n",numRows);
          if(numRows == 0)
          {
            sprintf(tmpp,"INSERT INTO %s_option (name,value) VALUES ('%s','%s')",configname,db_field[iii],db_value[iii]);
            printf(">%s<\n",tmpp);
            if(mysql_query(dbsocket,tmpp) != 0)
            {
              printf("ERROR in INSERT\n");
            }
          }
          else if(numRows == 1)
          {
            sprintf(tmpp,"UPDATE %s_option SET value='%s' WHERE name='%s'\n",configname,db_value[iii],db_field[iii]);
            printf(">%s<\n",tmpp);
            if(mysql_query(dbsocket,tmpp) != 0)
            {
              printf("ERROR in UPDATE\n");
            }
          }
          else
          {
            printf("ERROR !!\n");
          }

          mysql_free_result(result);
	    }
      }
	}

    /* disconnect from database */
    dbDisconnect(dbsocket);
  }

}




#include <et_private.h> 
#include <time.h> 
#include <sys/times.h>

#include "circbuf.h"
#include "largebuf.h"

typedef struct biger
{
  int id;
  LARGEBUF *glargein;  /* input data buffer */

  /* general info */
  int runNumber;
  char filename[128]; /* evio data file name (without extension) */

} BIGER;

/* big buffers to be used between 'coda_proc' and 'coda_net' */
static LARGEBUF *glargeBUF = NULL;

#define MYCLOCK NANOMICRO

#define DEBUG


#define NUM_ER_BUFS MAXTHREAD
#define ER_BUF_SIZE 2147000000 /* just short of 2GByte */
/*#define ER_BUF_SIZE 1024000000*/
/*#define ER_BUF_SIZE 512000000*/

#define BUFFERMARGIN  10000000  /* 10MB - must be bigger then maximum possible event size ! */

#define BUFFERSIZE (ER_BUF_SIZE/4) /*4194304*/ /* evio buffer size in words,
                            like 'bs' in 'dd if=/dev/zero of=/data/stage_in/out.1 bs=16M count=2000 oflag=direct' */

/* evio_thread structure, contains pointer to the glargeBUF */
static BIGER biger[MAXTHREAD];

void 
evio_thread(BIGER *bigerptrin)
{
  int length, status, fd, ifend, tmp;
  int i, ii, jj, len, lentot, nev, nevent;
  unsigned int lwd;
  unsigned int *largebuf;
  BIGER *bigerptr;
  int splitnb, bufindex;
  uint32_t *ptr;
  char str[32];
  char current_file[256];
  FILE *fdbin;
  TIMERL_VAR;

/* timing */
#ifndef Darwin
  hrtime_t start, end, time1, time2, icycle, cycle=100;
#endif


  /*
  printf("input: bigerptrin=0x%08x offsetin=0x%08x\n",
    bigerptrin,offsetin);
  */
  bigerptr = bigerptrin;

#ifdef Linux
  sprintf(str,"evio_thread_%d",bigerptr->id);
  printf("set pthread name to >%s<\n",str);fflush(stdout);
  prctl(PR_SET_NAME,str);
#endif

#ifdef DEBUG
  for(i=0; i<8*bigerptr->id+8; i++) printf(" ");
  printf("[%d] biger at 0x%08x, biger.glargein at 0x%08x (0x%08x)\n",bigerptr->id,
		 bigerptr, &(bigerptr->glargein), bigerptr->glargein);fflush(stdout);
#endif
  sleep(1);

  /*printf("evio_thread reached\n");fflush(stdout);*/
  nevent = 0;
  largebuf = NULL;
#ifndef Darwin
  icycle = time1 = time2 = 0;
#endif

  do
  {
    icycle ++;
    start = gethrtime();









if(bigerptr->id==0) {TIMERL_START;}
    /* grab buffer */
    largebuf = lb_read_grab(&(bigerptr->glargein),bigerptr->id);
if(bigerptr->id==0) {TIMERL_STOP(1,bigerptr->id);}








#ifndef DO_NOT_WRITE






#ifdef DEBUG
    for(i=0; i<8*bigerptr->id+8; i++) printf(" ");
    printf("[%d] evio_thread !!! lb_read(0x%08x), buflen=%d, buf# %d (%d), end=%d\n",
      bigerptr->id,&(bigerptr->glargein),largebuf[IBUFLEN],largebuf[IBUFNUM],largebuf[IBUFIND],largebuf[IBUFEND]);fflush(stdout);
#endif

    if(largebuf<0)
    {
      printf("[%d] evio_thread: ERROR: lb_read_grab return %d\n",bigerptr->id,largebuf);fflush(stdout);
      break;
    }
    else if(largebuf == NULL)
    {
      printf("[%d] evio_thread: INFO: lb_read_grab return on cleanup condition\n",bigerptr->id);fflush(stdout);
      break;
    }

    end = gethrtime();
    time1 += (end-start)/MYCLOCK;

    start = gethrtime();

    /* get info from arrieved buffer */
    splitnb  = largebuf[IBUFNUM];
    bufindex = largebuf[IBUFIND];
    ifend    = largebuf[IBUFEND];
    lentot   = largebuf[IBUFLEN];


    if(lentot>0)
	{

      /* open evio file */
      sprintf(current_file, "%s_%06d.evio.%d", bigerptr->filename, bigerptr->runNumber, splitnb);
#ifdef DEBUG
      for(i=0; i<8*bigerptr->id+8; i++) printf(" ");
	  printf("[%d] coda_er(evio_thread): Opening file : %s\n",bigerptr->id,current_file);fflush(stdout);
#endif

      fd = 0;
	  evOpen(current_file,"w",&fd);
      tmp = BUFFERSIZE;
      evIoctl(fd,"B",&tmp);

      /* write evio file */
      ii = 0;
      nev = 0;
      ptr = &largebuf[BUFHEAD];
#ifdef DEBUG
      for(i=0; i<8*bigerptr->id+8; i++) printf(" ");
      printf("[%d] evio_thread: begin: lentot=%d ii=%d, buf# %d (#d)\n",bigerptr->id,lentot,ii,splitnb,bufindex);
#endif






      while(ii<lentot)
	  {
        len = ptr[0];
	    if(len<=0)
	    {
          for(i=0; i<8*bigerptr->id+8; i++) printf(" ");
          printf("[%d] evio_thread: ERROR: len=%d, ii=%d, buf# %d (%d)\n",bigerptr->id,len,ii,splitnb,bufindex);
          exit(0);
	    }
	    /*printf("evio_thread: len=%d, ii=%d\n", len,ii);
        for(jj=0; jj<len; jj++)
	    {
          printf("evio_thread: data[%d]=0x%08x\n",jj,ptr[jj]);
	    }
	    */
        status = evWrite(fd, &ptr[1]);
        if(status!=0)
        {
          for(i=0; i<8*bigerptr->id+8; i++) printf(" ");
          printf("[%d] coda_er(evio_thread): ERROR: evWrite returns %d\n",bigerptr->id,status);fflush(stdout);
          return;
        }

        ptr += len;
        ii += len;
        nev ++;
        /*printf("evio_thread: middle: lentot=%d ii=%d\n",lentot,ii);*/
	  }





#ifdef DEBUG
      for(i=0; i<8*bigerptr->id+8; i++) printf(" ");
      printf("[%d] evio_thread: end: lentot=%d ii=%d nev=%d, buf# %d (%d)\n",bigerptr->id,lentot,ii,nev,splitnb,bufindex);
#endif

      /* close evio file */
      /*if(fd > 0) evClose(fd);*/
	  evClose(fd);

#ifdef DEBUG
      for(i=0; i<8*bigerptr->id+8; i++) printf(" ");
	  printf("[%d] coda_er(evio_thread): closed file: %s\n",bigerptr->id,current_file);fflush(stdout);
#endif

	  largebuf[IBUFLEN] = 0;
      largebuf[IBUFEND] = 0;

	} /*if(lentot>0)*/








#endif /*DO_NOT_WRITE*/








    /* release buffer */
    largebuf = lb_read_release(&(bigerptr->glargein),bigerptr->id);
    if(largebuf <0)
    {
      printf("[%d] evio_thread: ERROR: lb_read_release return %d\n",bigerptr->id,largebuf);fflush(stdout);
      break;
    }
    else if(largebuf == NULL)
    {
      printf("[%d] evio_thread: INFO: lb_read_release return on cleanup condition\n",bigerptr->id);fflush(stdout);
      break;
    }









/*timing */
    end = gethrtime();
    time2 += (end-start)/MYCLOCK;

    if(nevent != 0 && icycle >= cycle)
    {
#if 1
      printf("[%d] evio_thread:  waiting=%7llu    sending=%7llu microsec per event (nev=%d)\n",bigerptr->id,
        time1/nevent,time2/nevent,nevent/icycle);
#endif
      nevent = icycle = time1 = time2 = 0;
    }


    /* exit the loop if 'End' condition was received */
    if(ifend == 1)
    {
      printf("[%d] evio_thread: END condition received\n",bigerptr->id);fflush(stdout);
      for(jj=0; jj<1/*0*/; jj++) {printf("[%d] EVIO_THREAD GOT END %d SEC\n",bigerptr->id,jj);sleep(1);}
      break;
    }

  } while(1);






printf("[%d] cleanup +++++++++++++++++++++++++++++++++++++ 1\n",bigerptr->id);fflush(stdout);

  /* force 'big' buffer read/write methods to exit */

  /* tell ALL threads to exit ? */

  lb_cleanup(&(bigerptr->glargein));
  sleep(1);

printf("[%d] cleanup +++++++++++++++++++++++++++++++++++++ 2\n",bigerptr->id);fflush(stdout);

  printf("[%d] WRITE THREAD EXIT\n",bigerptr->id);fflush(stdout);
}






#define ET_EVENT_ARRAY_SIZE 100

extern char et_name[ET_FILENAME_LENGTH];
static et_stat_id  et_statid;

/*???*/
/*static*/ et_sys_id   et_sys;
/*static*/ et_att_id   et_attach;

static int         et_locality, et_init = 0, et_reinit = 0;

/* ET Initialization */    
int
er_et_initialize(void)
{
  et_statconfig   sconfig;
  et_openconfig   openconfig;
  int             status;
  struct timespec timeout;

  timeout.tv_sec  = 2;
  timeout.tv_nsec = 0;

  /* Normally, initialization is done only once. However, if the ET
   * system dies and is restarted, and we're running on a Linux or
   * Linux-like operating system, then we need to re-initalize in
   * order to reestablish the tcp connection for communication etc.
   * Thus, we must undo some of the previous initialization before
   * we do it again.
   */
  if(et_init > 0)
  {
    /* unmap shared mem, detach attachment, close socket, free et_sys */
    et_forcedclose(et_sys);
  }
  
  printf("er_et_initialize: start ET stuff\n");

  if(et_open_config_init(&openconfig) != ET_OK)
  {
    printf("ERROR: er ET init: cannot allocate mem to open ET system\n");
    return ER_ERROR;
  }
  et_open_config_setwait(openconfig, ET_OPEN_WAIT);
  et_open_config_settimeout(openconfig, timeout);
  if(et_open(&et_sys, et_name, openconfig) != ET_OK)
  {
    printf("ERROR: er ET init: cannot open ET system\n");
    return ER_ERROR;
  }
  et_open_config_destroy(openconfig);

  /* set level of debug output */
  et_system_setdebug(et_sys, ET_DEBUG_ERROR);

  /* where am I relative to the ET system? */
  et_system_getlocality(et_sys, &et_locality);

  et_station_config_init(&sconfig);
  et_station_config_setselect(sconfig,  ET_STATION_SELECT_ALL);
  et_station_config_setblock(sconfig,   ET_STATION_BLOCKING);
  et_station_config_setuser(sconfig,    ET_STATION_USER_MULTI);
  et_station_config_setrestore(sconfig, ET_STATION_RESTORE_OUT);
  et_station_config_setprescale(sconfig,1);

  if((status = et_station_create(et_sys, &et_statid, "TAPE", sconfig)) < 0)
  {
    if (status == ET_ERROR_EXISTS) {
      printf("er ET init: station exists, will attach\n");
    }
    else
    {
      et_close(et_sys);
      et_station_config_destroy(sconfig);
      printf("ERROR: er ET init: cannot create ET station (status = %d)\n",
        status);
      return(ER_ERROR);
    }
  }
  et_station_config_destroy(sconfig);

  if (et_station_attach(et_sys, et_statid, &et_attach) != ET_OK) {
    et_close(et_sys);
    printf("ERROR: er ET init: cannot attached to ET station\n");
    return ER_ERROR;
  }

  et_init++;
  et_reinit = 0;
  printf("er ET init: ET fully initialized\n");
  return ER_OK;
}



int
ER_constructor()
{
  static ER_priv ERP;
  uint64_t maxNeededBytes = 0;
  uint64_t maxAvailBytes = 0;
  uint64_t tsendBufSize = 0;

  localobject->privated = (void *) &ERP;

  bzero ((char *) &ERP,sizeof(ERP));

  ERP.split = 512*1024*1024;


  memset((char *) &biger, 0, sizeof(BIGER));

  /*************************/
  /* allocate buffer space */

  /* memory size we need */
  maxNeededBytes = ER_BUF_SIZE*NUM_ER_BUFS;
  printf("min=0x%08x (bufs 0x%08x x %d)\n",maxNeededBytes,ER_BUF_SIZE,NUM_ER_BUFS);

  maxAvailBytes = ER_BUF_SIZE * NUM_ER_BUFS;
  tsendBufSize = ER_BUF_SIZE;
  printf("INFO: wants=0x%08x, maxAvailBytes=0x%08x, send=0x%08x\n",maxNeededBytes,maxAvailBytes,tsendBufSize);

  /* create input 'big' buffer pools for 'proc' and 'net'; input to the 'net' will be
  used as output from 'proc' if both of them are running on host */


  /*************************************/
  /* intermediate buffer; give it id=9 */
  glargeBUF = lb_new(9,NUM_ER_BUFS,tsendBufSize);
  if(glargeBUF == NULL)
  {
    printf("ERROR in lb_new: glargeBUF allocation FAILED\n");
    return(CODA_ERROR);
  }
  else
  {
    printf("lb_new: glargeBUF allocated at 0x%08x\n",glargeBUF);
  }



  tcpState = DA_BOOTED;
  if(codaUpdateStatus("booted") != ER_OK) return(ER_ERROR);

  tcpServer(localobject->name, mysql_host); /*start server to process non-coda commands sent by tcpClient*/

  return(ER_OK);
}





/*********************************************************/
/*********************************************************/
/******************** CODA format ************************/


int 
gotControlEvent(et_event **pe, int size)
{
    int i;
    
    for (i=0; i<size; i++)
    {
	  /*
if(pe[i]->control[0] & 0x40) printf("coda_ebc: syncFlag detected, control[0]=0x%08x\n",pe[i]->control[0]);
	  */

      if ((pe[i]->control[0] == EV_ER_PRESTART) || (pe[i]->control[0] == EV_ER_END))
      {
        return 1;
      }
    }
    return 0;
}


/*
 largebuf[IBUFLEN] - total length in words
 largebuf[IBUFNUM] - file name extension number
 largebuf[IBUFIND] - buffer number
 largebuf[IBUFEND] - end condition

 largebuf[BUFHEAD] - largebuf[..] - first event, starting from event length

 ...............

*/

static unsigned int *largebufout;
static unsigned int *buffer;


/* if force_write==1, release buffer immediately */

int 
outputEvents(ERp erp, et_event **pe, int start, int stop, int force_write)
{
  int handle1, i, j, buflen, bufnum, len, tmp, status=0;
  unsigned int *ptr;
  TIMERL_VAR;


  /*5GB/sec
#ifdef DO_NOT_OUTPUT
  return(CODA_OK);
#endif
  */

  /*printf("outputEvents: start=%d stop=%d\n",start,stop);fflush(stdout);*/

  /* evio file output */
#ifdef DEBUG__
  printf("start=%d, stop=%d\n",start,stop);
#endif

  for (i=start; i<=stop; i++)
  {
    len = pe[i]->length>>2;
#ifdef DEBUG__
    printf("Got event i=%d of length %d words\n",i,len);
#endif

	erp->object->nlongs += len;
    erp->nlongs += len;
    erp->object->nevents++;
    erp->nevents++;

    et_event_getlength(pe[i], &len); /* return bytes ? */
#ifdef DEBUG__
    printf("Second event length %d bytes (%d words)\n",len,len>>2);
#endif
    ptr = (unsigned int *)pe[i]->pdata;
    ptr += 8; /* skip record header, evWrite() will produce it again */
    len = len >> 2; /* bytes -> words */
    len -= 8; /* remove evio record header length */
    len ++; /* add length itself to the word count */
#ifdef DEBUG__
    printf("Final event length %d words\n",len);
#endif





#ifndef DO_NOT_OUTPUT



TIMERL_START;




    /****************************************************************/
    /* assume that largebufout and buffer are set at that point !!! */
    /****************************************************************/

	if(len<=0)
	{
      printf("outputEvents: ERROR: len=%d, i=%d, start=%d, stop=%d, buf# %d (%d)\n",len,i,start,stop,(erp->splitnb-1),buffer[IBUFLEN]);
      /*exit(0);*/
	}
	else
	{
      /* add one event to the big buffer */
      buffer[IBUFLEN] += len; /* update total length of the big buffer */
	  /*printf("outputEvents: buffer[IBUFLEN]=%d\n", buffer[IBUFLEN]);*/
      *largebufout++ = len; /* write length of event area - can be bigger the event ! */
	  /*printf("outputEvents: len=%d\n", len);*/
      for(j=0; j<(len-1); j++)
	  {
        *largebufout++ = *ptr++; /* copy event into big buffer */
        /*printf("outputEvents: data[%d]=0x%08x\n",j,*(largebufout-1));*/
	  }
     /*printf("coda_er:outputEvents(): buffer[IBUFLEN]=%d\n",buffer[IBUFLEN]);fflush(stdout);*/




	  /* if big buffer is full, send it and get next one */
      /* if force_write is set, send it but do NOT get new one */
      if( (buffer[IBUFLEN]>((ER_BUF_SIZE-BUFFERMARGIN)>>2)) || (force_write==1) )
	  {
        buflen = buffer[IBUFLEN];
        bufnum = buffer[IBUFNUM];
        buffer[IBUFEND] = force_write; /* set 'end' condition in buffer header */

/*TIMERL_START;*/
        largebufout = lb_write_release(&glargeBUF);
        if(largebufout < 0)
        {
          printf("coda_er:outputEvents: ERROR in lb_write_release: FAILED\n");fflush(stdout);
          exit(0);
        }
        else if(largebufout == NULL)
        {
          printf("coda_er:outputEvents: INFO: lb_write_released returned on cleanup condition\n");        
          return(CODA_ERROR);
        }
        else
	    {
          /*printf("coda_er:outputEvents: INFO: lb_write_released returned 0x%08x\n",largebufout)*/;
	    }
/*TIMERL_STOP(10,8);*/ /* 2-3us*/

/*TIMERL_START;*/
        if(force_write==0)
	    {
          largebufout = lb_write_grab(&glargeBUF);
/*TIMERL_STOP(10,9);*/ /*1us*/
          if(largebufout < 0)
          {
            printf("coda_er:outputEvents: ERROR in lb_write_grab: FAILED\n");fflush(stdout);
            exit(0);
          }
          else if(largebufout == NULL)
          {
            printf("coda_er:outputEvents: INFO: lb_write_grab returned on cleanup condition\n");        
            return(CODA_ERROR);
          }
          else
	      {
            buffer = largebufout;
            buffer[IBUFLEN] = 0;
            buffer[IBUFEND] = 0;
            buffer[IBUFNUM] = erp->splitnb++; /* put file extension into big buffer so writing thread can make correct file name */;
            largebufout += BUFHEAD;
		    /*
            printf("outputEvents(): lb_write_grab called Ok, released buflen=%d words, buf# %d, got new largebufout=0x%08x, buf# %d (%d)\n",
			   buflen,bufnum,largebufout,buffer[IBUFNUM],buffer[IBUFIND]);fflush(stdout);
            update_database(erp);
		    */
	      }
	    }
        else
	    {
/*TIMERL_STOP(10,9);*/
          printf("coda_er:outputEvents: INFO: force_write!=0, do NOT grab buffer, return(1)\n");fflush(stdout);
          return(1); /* can return OK ??? */
	    }

	  }



	} /*len<=0)*/




	TIMERL_STOP(100000,7); /* 9us with -O2, 26-32us with -g */

#endif /*DO_NOT_OUTPUT*/







  } /* loop over events in chunk */


  /*printf("outputEvents: done\n");fflush(stdout);*/

  return(CODA_OK);
}




int 
CODA_write_event(ERp erp)
{
  const int prestartEvent=EV_ER_PRESTART, endEvent=EV_ER_END, true=1, false=0;
  int status1, status2;
  int i, ii, status, len, done=false, start, stop;
  et_event *pe[ET_EVENT_ARRAY_SIZE];
  struct timespec waitfor;
  struct tms tms_start, tms_end;
  clock_t start_ticks, stop_ticks;
  int force_write;
  int nevents;
  /*TIMERL_VAR;*/

  waitfor.tv_sec  = 0;
  waitfor.tv_nsec = 10000000L; /* .010 sec */

  struct timespec deltatime;
  deltatime.tv_sec  = 0;
  deltatime.tv_nsec = 1000000000L; /* 1000 millisec */


  if (!et_alive(et_sys))
  {
    printf("CODA_write_event: not attached to ET system\n");
    et_reinit = 1;
    return(0);
  }

  if( (start_ticks = times(&tms_start)) == -1)
  {
    printf("CODA_write_event: ERROR getting start time\n");
  }


  do
  {

try_again:

    force_write = 0;

    /*printf("get events from ET\n");*/
/*TIMERL_START;*/
    /*ET_SLEEP,ET_TIMED,ET_ASYNC*/
    /*status1 = et_events_get(et_sys, et_attach, pe, ET_SLEEP, NULL, ET_EVENT_ARRAY_SIZE, &nevents);*/
    status1 = et_events_get(et_sys, et_attach, pe, ET_TIMED, &deltatime, ET_EVENT_ARRAY_SIZE, &nevents);
    /*status1 = et_events_get(et_sys, et_attach, pe, ET_ASYNC, NULL, ET_EVENT_ARRAY_SIZE, &nevents);*/
    if(status1 < 0)
	{
#ifdef DEBUG
      printf("et_events_get(ET_TIMED) returned %d - trying to get events again\n",status1);
#endif
      goto try_again;
	}
/*TIMERL_STOP(1,8);*/


    /* if no events or error ... */
    if ((nevents < 1) || (status1 != ET_OK))
    {
      /* if status1 == ET_ERROR_EMPTY or ET_ERROR_BUSY, no reinit is necessary */
      
      /* will wake up with ET_ERROR_WAKEUP only in threaded code */
      if (status1 == ET_ERROR_WAKEUP)
      {
        printf("CODA_write_event: forcing writer to quit read, status = ET_ERROR_WAKEUP\n");
      }
      else if (status1 == ET_ERROR_DEAD)
      {
        printf("CODA_write_event: forcing writer to quit read, status = ET_ERROR_DEAD\n");
        et_reinit = 1;
      }
      else if (status1 == ET_ERROR)
      {
        printf("CODA_write_event: error in et_events_get, status = ET_ERROR \n");
        et_reinit = 1;
      }
      done = true;
    }
    else    /* if we got events ... */
    {
      /* by default (no control events) write everything */
      start = 0;
      stop  = nevents - 1;
      force_write = 0;

#ifdef DEBUG__
      printf("GOT %d events\n",nevents);
      for(i=0; i<nevents; i++)
	  {
        printf("  [%2d] len=%d bytes\n",i,pe[i]->length);
        /*if(pe[i]->length<32) exit(0);*/
	  }
#endif

      /* if we got control event(s) ... */
      if (gotControlEvent(pe, nevents))
      {
        /* scan for prestart and end events */
        for (i=0; i<nevents; i++)
        {
	      if (pe[i]->control[0] == prestartEvent)
          {
	        printf("Got Prestart Event!!\n"); /* sergey: is it before clearing PrestartCount in ER prestart, or after ? */
			mbytes_in_current_run = 0;
            nevents_in_current_run = 0;

	        /* look for first prestart */
	        if (PrestartCount == 0)
            {
	          /* ignore events before first prestart */
	          start = i;
	          if (i != 0)
              {
	            printf("CODA_write_event: ignoring %d events before Prestart, setting start=%d\n",i,start);
	          }
	        }
            PrestartCount++;
	      }
	      else if (pe[i]->control[0] == endEvent)
          {
            printf("End Event: erp->nend=%d !!!!!!!!!!!!!!!!!!!!!!!!!!!\n",erp->nend);
            erp->nend--;
	        /* ignore events after last end event & quit */
	        if (erp->nend <= 0)
            {
              printf("End Event: erp->nend=%d -> ignore events after End !!!!!!!!!!!!!!!!!!!!!!!\n",erp->nend);
	          stop = i; /* last event to be written */
	          done = true;
              printf("End Event: forcing writing, start=%d, stop=%d\n",start,stop);
              force_write = 1;
	        }
            printf("Got End event, Need %d more\n",erp->nend);
	      }
        }
      } 
      
      /* write events to output (ignore events before first prestart) */
      if (PrestartCount != 0)
      {
        if (outputEvents(erp, pe, start, stop, force_write) != 0)
        {
	      /* error writing coda file so quit */
	      printf("ERROR: Error writing events... Cancel ET read loop!\n");
          done = true;
        }
        else
		{
          /*printf("Wrote %d events\n",nevents)*/;
		}
      }

      /* cleanup all flags to make sure outputEvents() never called again, until next Prestart arrives */
      if(force_write) PrestartCount = 0;
      force_write = 0;

      /* put et events back into system */
      /*printf("put events back to ET\n");*/
      status2 = et_events_put(et_sys, et_attach, pe, nevents);
      if (status2 != ET_OK)
      {
	    printf("CODA_write_event: ERROR in et_events_put, status = %i \n",status2);
        et_reinit = 1;
        done = true;
      }

    }






    /* check timeout */

    if( (stop_ticks = times(&tms_end)) == -1)
    {
      printf("CODA_write_event: ERROR getting stop time\n");
    }
    else
    {
      /*sergey: why HZ ??? it does not defined in Darwin anyway .. */
#ifndef Darwin
      if( ((stop_ticks-start_ticks)/60) > ER_WRITE_LOOP_TIMEOUT)
      {
#else
      if( ((stop_ticks-start_ticks)/CLK_TCK) > ER_WRITE_LOOP_TIMEOUT)
      {
#endif
	    printf("CODA_write_event: WARN: ER is backed up! This may be causing system deadtime\n");
	    done = true;
      }
    }


  } while(done == false);



 
  if(erp->nend <= 0)
  { 
    printf("CODA_write_event: return(0), will exit loop and stop pulling events from ET\n");
    return(0); /* will break loop over CODA_write_event */
  }


  printf("CODA_write_event: return(1)\n");
  return(1); /* will continue loop CODA_write_event */
}



/* main thread getting events from ET and filling large buffers */
void *
er_write_thread(objClass object)
{
  ERp erp;
  int ix, status = 0;

#ifdef Linux
  prctl(PR_SET_NAME,"er_write_thread");
#endif

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &ix);
  
  erp = (ERp) object->privated;


  do
  {
    status = CODA_write_event(erp);

  } while (status);

  printf("er_write_thread loop ended, status=%d\n",status);

  pthread_exit(0);
}



int
erFlushET()
{
  objClass object = localobject;

  ERp erp = (ERp) object->privated;
  int stat1 = 0;
  int stat2 = 0;
  int evCnt = 0;

  et_event *pe[ET_EVENT_ARRAY_SIZE];
  int nevents=0, try=0, try_max=5;


  /* - wait for ET system here since this is first item in prestart
   * - wait 5 sec min or 2X monitor period
   */
  if (!et_alive(et_sys))
  {
    int waitforET = 2*(ET_MON_SEC + 1);

    /* if this is Linux ... */
    if (et_locality == ET_LOCAL_NOSHARE)
    {
      /* The ET system has died and our tcp connection has
       * been broken as evidenced by the failure of et_alive.
       * So reinitialize ET to reestablish the link.
       */
      if (er_et_initialize() != ER_OK)
      {
        printf("ERROR: ER_flush: cannot reinitalize ET system\n");
        return(0);
      }
    }
    /* if Solaris, wait for ET system to come back */
    else
    {
      if (waitforET < 5) waitforET = 5;
      sleep(waitforET);
      if (!et_alive(et_sys))
      {
	    printf("ER_flush: ET system is dead\n");
	    return(0);
      }
    }
  }



  do
  {
    stat1 = et_events_get(et_sys, et_attach, pe, ET_ASYNC, NULL, ET_EVENT_ARRAY_SIZE, &nevents);

    if(stat1 == ET_OK && (nevents > 0))
    {
      /* put events back */
      stat2 = et_events_put(et_sys, et_attach, pe, nevents);
      printf("ER_flush: flushed %d events from ET system\n", nevents);
    }
    else if(stat1 == ET_ERROR_EMPTY)
    {
      printf("ER_flush: no (more) ET events to flush\n");
    }
    else if (stat1 == ET_ERROR_BUSY)
    {
      printf("ER_flush: ET is busy, wait and try again\n");
      sleep(1);
      if(try++ < try_max)
      {
        stat1 = ET_OK;
      }
      else
      {
        printf("ER_flush: ET station is too busy, can't flush\n");
      }
    }
    else
    {
      printf("ER_flush: error in getting ET events\n");
      et_reinit = 1;
      return(0);
    }
  } while(stat1 == ET_OK);

  return(ER_OK);
}





int
erDaqCmd(char *param)
{
  objClass object = localobject;

  pthread_attr_t detached_attr;
  int i, status;

  ERp erp = (ERp) object->privated;
  switch(param[0])
  {
  case 'o': /*open*/

    erp->nlongs = 0;
    erp->nevents = 0;
    erp->splitnb = 0;

    erp->write_thread = NULL; /*sergey: will check it*/
    printf("starting write thread 1 ..\n");fflush(stdout);
    pthread_attr_init(&detached_attr);
    pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(&detached_attr, PTHREAD_SCOPE_SYSTEM);
    status = pthread_create( &erp->write_thread, &detached_attr,
                    (void *(*)(void *)) er_write_thread, (void *) object);
    if(status!=0)
    {
      printf("erDaqCmd1: ERROR: pthread_create returned %d - exit\n",status);fflush(stdout);
      perror("erDaqCmd1: pthread_create");
      exit(0); 
    }
    else
    {
      printf("erDaqCmd1: pthread_create succeeded\n");fflush(stdout);
    }


    /* input to evio_thread goes from glargeBUF */

    for(i=0; i<MAXTHREAD; i++)
	{
      /*HACK
      if(i%2) strcpy(erp->filename,"/data1/stage_in/test");
      else    strcpy(erp->filename,"/data2/stage_in/test");
	  HACK*/

      strcpy(biger[i].filename,erp->filename);
      biger[i].runNumber = object->runNumber;
      biger[i].glargein = glargeBUF; /* every 'biger[]' contains pointer to the same 'glargeBUF' */
      biger[i].id = i;

      erp->evio_thread[i] = NULL; /*sergey: will check it*/
      printf("\n --> starting evio thread [%d]\n\n",biger[i].id);fflush(stdout);
      pthread_attr_init(&detached_attr);
      pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
      pthread_attr_setscope(&detached_attr,PTHREAD_SCOPE_SYSTEM/*PTHREAD_SCOPE_PROCESS*/);
      status = pthread_create( &erp->evio_thread[i], &detached_attr,
		                    (void *(*)(void *)) evio_thread, (void *) &biger[i]);
      if(status!=0)
      {
        printf("erDaqCmd2: ERROR: pthread_create returned %d - exit\n",status);fflush(stdout);
        perror("erDaqCmd2: pthread_create");
        exit(0); 
      }
      else
      {
        printf("erDaqCmd2: pthread_create succeeded\n");fflush(stdout);
      }
	}


    /* get first large buffer */
    largebufout = lb_write_grab(&glargeBUF);
    if(largebufout<0)
    {
      printf("erDaqCmd: ERROR: lb_write_current returned %d - exit\n",largebufout);
      exit(0);
    }
    else if(largebufout == NULL)
    {
      printf("erDaqCmd: INFO: lb_write_current returned on cleanup condition\n");
      exit(0);
    }
    buffer = largebufout;
    printf("erDaqCmd: largebufout=0x%08x, initialize output buffer header\n",largebufout);fflush(stdout);
    buffer[IBUFLEN] = 0;
    buffer[IBUFEND] = 0;
    buffer[IBUFNUM] = erp->splitnb++; /* put file extension into big buffer so writing thread can make correct file name */;
    largebufout += BUFHEAD;


    break;

  case 'c': /*close*/
    {
      void *status;


      printf("waiting for evio thread 1 ..\n");fflush(stdout);

      for(i=0; i<MAXTHREAD; i++)
	  {
	    if(erp->evio_thread[i] > 0)
	    {
          pthread_join(erp->evio_thread[i], &status);
          erp->evio_thread[i] = NULL; /*sergey: will check it*/
          printf("evio_thread[%d] is done\n",i);fflush(stdout);
	    }
	  }

      printf("waiting for write thread 1 ..\n");fflush(stdout);

      et_wakeup_attachment(et_sys, et_attach);
      printf("waiting for write thread 2 .. erp->write_thread=0x%08x\n",erp->write_thread);fflush(stdout);

      /* if thread was started, cancel it */
	  if(erp->write_thread > 0)
	  {
        printf("update_database called ..\n");
	    update_database(erp);
        printf(".. update_database done\n");

        /*pthread_cancel(erp->write_thread);*/

        printf("waiting for write thread 3 ..\n");fflush(stdout);

        pthread_join(erp->write_thread, &status);
        erp->write_thread = NULL; /*sergey: will check it*/
        printf("write thread is done\n");fflush(stdout);

	  }


    }
    break;


  case 'r': /*go, resume*/
    if(PrestartCount > 0)
    {
      erp->nend = PrestartCount; /*sergey: ??? */
    }
    else
    {
      erp->nend = 1;
    }
    printf("INFO: ER will require %d End event(s) to close output\n",erp->nend);
    break;


  case 'p': /*pause*/
    break;




  default:
    printf("erDaqCmd: unknown param=>%s<\n",param);
  }
  return(ER_OK);
}





/*********************************************************/
/********************** transitions **********************/
/*********************************************************/

int
codaInit(char *conf)
{
  if(codaUpdateStatus("configuring") != CODA_OK)
  {
    return(CODA_ERROR);
  }

  UDP_start();

  if(codaUpdateStatus("configured") != CODA_OK)
  {
    return(CODA_ERROR);
  }

  return(CODA_OK);
}


int
codaDownload(char *conf)
{
  objClass object = localobject;

  ERp erp = (ERp) object->privated;
  static char tmp[1000];
  static char tmp2[1000];
  int  i, ix;  
  int  listArgc;
  char listArgv[LISTARGV1][LISTARGV2];

  MYSQL *dbsock;
  char tmpp[1000];

  if(glargeBUF != NULL)
  {
    lb_init(&glargeBUF);
  }
  


  erp->object = object;
  erp->write_thread = NULL; /*sergey: will check it*/
  for(i=0; i<MAXTHREAD; i++) erp->evio_thread[i] = NULL; /*sergey: will check it*/

  /***************************************************/
  /* extract all necessary information from database */


  /*****************************/
  /*****************************/
  /* FROM CLASS (former conf1) */

  strcpy(configname,conf); /* Sergey: save CODA configuration name */

  UDP_start();

  tcpState = DA_DOWNLOADING;
  if(codaUpdateStatus("downloading") != ER_OK) return(ER_ERROR);

  /* connect to database */
  dbsock = dbConnect(mysql_host, expid);

  sprintf(tmp,"SELECT value FROM %s_option WHERE name='SPLITMB'",
    configname);
  if(dbGetInt(dbsock, tmp, &erp->split)==ER_ERROR)
  {
    erp->split = 2047 << 20;
    printf("cannot get SPLITMB, set erp->split=%d Bytes\n",erp->split);
  }
  else
  {
    printf("get erp->split = %d MBytes\n",erp->split);
    erp->split = erp->split<<20;
    printf("set erp->split = %d Bytes\n",erp->split);
  }

  sprintf(tmp,"SELECT value FROM %s_option WHERE name='RECL'",
    configname);
  if(dbGetInt(dbsock, tmp, &erp->record_length)==ER_ERROR)
  {
    erp->record_length = /*32768*//*1048576*/16777216;
    printf("cannot get RECL, set erp->record_length=%d\n",erp->record_length);
  }
  else
  {
    printf("get erp->record_length = %d\n",erp->record_length);
  }


  /* do not need that !!!???
  sprintf(tmp,"SELECT value FROM %s_option WHERE name='EvDumpLevel'",
    configname);
  if(dbGetInt(dbsock, tmp, &erp->log_EvDumpLevel)==ER_ERROR)
  {
    erp->log_EvDumpLevel = 0;
    printf("cannot get EvDumpLevel, set erp->log_EvDumpLevel=%d\n",erp->log_EvDumpLevel);
  }
  else
  {
    printf("get erp->log_EvDumpLevel = %d\n",erp->log_EvDumpLevel);
  }
  */


  /* do not need that !!!???
  sprintf(tmp,"SELECT inputs FROM %s WHERE name='%s'",configname,object->name);
  if(dbGetStr(dbsock, tmp, tmpp)==ER_ERROR)
  {
    printf("cannot get 'inputs' from table>%s< for the name>%s<\n",configname,object->name);
  }
  else
  {
    printf("inputs >%s<\n",tmpp);
  }
  */


  /* do not need that !!!???
  sprintf(tmp,"SELECT outputs FROM %s WHERE name='%s'",configname,object->name);
  if(dbGetStr(dbsock, tmp, tmpp)==ER_ERROR)
  {
    printf("cannot get 'outputs' from table>%s< for the name>%s<\n",configname,object->name);
    return(ER_ERROR);
  }
  */

  /* default output to evio */
  printf("evio format will be used\n");

  sprintf(tmp,"SELECT value FROM %s_option WHERE name='dataFile'",configname);
  if(dbGetStr(dbsock, tmp, tmpp)==ER_ERROR)
  {
    printf("cannot get 'dataFile' from table >%s_option<\n",configname);
    return(ER_ERROR);
  }
  else
  {
    strcpy(erp->filename,tmpp);
    printf("got erp->filename >%s<\n",erp->filename);
  }



  /*****************************/
  /*****************************/
  /*****************************/

  printf("INFO: Downloading configuration '%s'\n", configname);

  /* Get the list of readout-lists from the database */
  sprintf(tmpp,"SELECT code FROM %s WHERE name='%s'",configname,object->name);
  if(dbGetStr(dbsock, tmpp, tmp)==ER_ERROR) return(ER_ERROR);
printf("++++++======>%s<\n",tmp);


  /* disconnect from database */
  dbDisconnect(dbsock);


  /* Decode configuration string */
  listArgc = 0;
  if(!((strcmp (tmp, "{}") == 0)||(strcmp (tmp, "") == 0)))
  {
    if(listSplit1(tmp, 1, &listArgc, listArgv)) return(ER_ERROR);
    for(ix=0; ix<listArgc; ix++) printf("nrols [%1d] >%s<\n",ix,listArgv[ix]);
  }
  else
  {
    printf("download: do not split list >%s<\n",tmp);
  }

  /* If we need to initialize, reinitialize, or
   * if et_alive fails on Linux, then initialize.
   */
  if( (et_init == 0)   ||
      (et_reinit == 1) ||
      ((!et_alive(et_sys)) && (et_locality == ET_LOCAL_NOSHARE))
     )
  {
    if(er_et_initialize() != ER_OK)
    {
      printf("ERROR: er download: cannot initalize ET system\n");
      return(ER_ERROR);
    }
  }

  tcpState = DA_DOWNLOADED;
  if(codaUpdateStatus("downloaded") != ER_OK) return(ER_ERROR);

  return(ER_OK);
}

int
codaPrestart()
{
  MYSQL *dbsock;
  char tmpp[1000];

  objClass object = localobject;

  ERp erp = (ERp) object->privated;

  printf("INFO: Prestarting\n");

  erFlushET();
  erp->nend = 1;

  /* connect to database */
  dbsock = dbConnect(mysql_host, expid);

  /* Get the run number */
  sprintf(tmpp,"SELECT runNumber FROM sessions WHERE name='%s'",session);
  if(dbGetInt(dbsock,tmpp,&(object->runNumber))==ER_ERROR) return(ER_ERROR);

  /* DO WE NEED TO GET runType AS WELL ??? */

  /* disconnect from database */
  dbDisconnect(dbsock);

  printf("INFO: prestarting,run %d, type %d\n",
    object->runNumber, object->runType);

  PrestartCount = 0;
  object->nevents = 0;
  object->nlongs = 0;

  erDaqCmd("open");

  tcpState = DA_PAUSED;
  codaUpdateStatus("paused");

  return(ER_OK);
}

int
codaGo()
{
  erDaqCmd("resume");
  tcpState = DA_ACTIVE;
  codaUpdateStatus("active");
  return(ER_OK);
}

int
codaEnd()
{
  int i;

  erDaqCmd("close");  /* wait for all threads to finish inside */

  /* cleanup all large buffers - SHOULD IT BE CLEANED UP ALREADY BY THE THREAD WHO RECEIVED 'END' ??? */
  lb_cleanup(&glargeBUF);

  tcpState = DA_DOWNLOADED;
  codaUpdateStatus("downloaded");
  return(ER_OK);  
}

int
codaPause()
{
  erDaqCmd("pause");
  tcpState = DA_PAUSED;
  codaUpdateStatus("paused");
  return(ER_OK);
}

int
codaExit()
{
  /*Sergey: if end failed, must do it here !!!*/erDaqCmd("close");
  tcpState = DA_CONFIGURED;
  codaUpdateStatus("configured");
  /*
  UDP_reset();
  */
  return(ER_OK);
}


/****************/
/* main program */
/****************/

void
main (int argc, char **argv)
{
  CODA_Init(argc, argv);

ER_constructor();

  /* CODA_Service ("ROC"); */
  CODA_Execute ();
}

#endif
