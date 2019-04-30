
/* coda_l3.c - CODA level3 */

#include <stdio.h>


#if defined(Linux_armv7l)

int
main()
{
  printf("coda_l3 is dummy for ARM etc\n");
}

#else

/* INCLUDES */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/times.h>

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
#include "rolInt.h"
#include "da.h"
#include "libdb.h"
#include "et_private.h"


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef RESTORE_OLD_SPEC_EVENT_CODING
#define EV_ER_PRESTART (EV_PRESTART-0x80)
#define EV_ER_END (EV_END-0x80)
#else
#define EV_ER_PRESTART EV_PRESTART
#define EV_ER_END EV_END
#endif

/* Define time limit for staying in write loop (seconds) */
#define ER_WRITE_LOOP_TIMEOUT   10


#define L3LIB


#if 0
/*activate NOT MULTITHREADED part !!!*/
//#define L3NOTHREADS

/* activate corrections for helicity delayed-reporting */
#define DELREP

#include "sinclude/delrep.c"
#endif

#include "helicity.h"




typedef int (*IFUNCPTR) ();

typedef struct ETpriv *ETp;
typedef struct ETpriv
{
  objClass object;

  /* params */
  int ngroups;
  int nevents;
  int event_size;
  int serverPort;
  int udpPort;
  int deleteFile;
  int noDelay;
  int maxNumStations;
  int sendBufSize;
  int recvBufSize;
  int et_verbose;
  int nend;
  /*int*/long nlongs;

  pthread_t process_thread;

  /* variables */
  char          host[ET_FILENAME_LENGTH];
  char          et_filename[ET_FILENAME_LENGTH];
  char          et_name[ET_FILENAME_LENGTH];
  char          mcastAddr[ET_IPADDRSTRLEN];
  int           sig_num;
  sigset_t      sigblockset;
  sigset_t      sigwaitset;
  et_sysconfig  config;
  et_sys_id     id;

} ET_priv;

static ET_priv ETP;


static int mbytes_in_current_run;
static int nevents_in_current_run;
static int force_exit = 0;

static int PrestartCount = 0;
/*static*/extern objClass localobject;
extern char configname[128]; /* coda_component.c */
extern char *mysql_host; /* coda_component.c */
extern char *expid; /* coda_component.c */
/*extern char *session;*/ /* coda_component.c */
#define L3_ERROR 1
#define L3_OK 0



int listSplit1(char *list, int flag,
           int *argc, char argv[LISTARGV1][LISTARGV2]);



/* Define time limit for staying in write loop (seconds) */
#define L3_WRITE_LOOP_TIMEOUT   10



#define MAXBUF 100000
static unsigned int hpsbuf[MAXBUF];

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

#define ET_EVENT_ARRAY_SIZE 100

extern char et_name[ET_FILENAME_LENGTH];
static et_stat_id  et_statid;

/*???*/
/*static*/ et_sys_id   et_sys;
/*static*/ et_att_id   et_attach;

static int         et_locality, et_init = 0, et_reinit = 0;

/* ET Initialization */    
int
l3_et_initialize(void)
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
  
  printf("l3_et_initialize: start ET stuff\n");

  if(et_open_config_init(&openconfig) != ET_OK)
  {
    printf("ERROR: l3 ET init: cannot allocate mem to open ET system\n");
    return L3_ERROR;
  }
  et_open_config_setwait(openconfig, ET_OPEN_WAIT);
  et_open_config_settimeout(openconfig, timeout);
  if(et_open(&et_sys, et_name, openconfig) != ET_OK)
  {
    printf("ERROR: l3 ET init: cannot open ET system\n");
    return L3_ERROR;
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

  if((status = et_station_create(et_sys, &et_statid, "L3", sconfig)) < 0)
  {
    if (status == ET_ERROR_EXISTS)
    {
      printf("l3 ET init: station exists, will attach\n");
    }
    else
    {
      et_close(et_sys);
      et_station_config_destroy(sconfig);
      printf("ERROR: l3 ET init: cannot create ET station (status = %d)\n",status);
      return(L3_ERROR);
    }
  }
  et_station_config_destroy(sconfig);

  if (et_station_attach(et_sys, et_statid, &et_attach) != ET_OK)
  {
    et_close(et_sys);
    printf("ERROR: l3 ET init: cannot attached to ET station\n");
    return L3_ERROR;
  }

  et_init++;
  et_reinit = 0;
  printf("l3 ET init: ET fully initialized\n");

  return(L3_OK);
}




int
l3Constructor()
{
  localobject->privated = (void *) &ETP;
  bzero ((char *) &ETP,sizeof(ETP));

  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;




  tcpState = DA_BOOTED;
  if(codaUpdateStatus("booted") != L3_OK) return(L3_ERROR);

  tcpServer(localobject->name, mysql_host); /*start server to process non-coda commands sent by tcpClient*/
}


int
l3Destructor()
{
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;

#if 0
  printf("Closing ET system\n");
  et_system_close(etp->id);
#endif

}

/***************************************************************************************/
/***************************************************************************************/
/***************************************************************************************/


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


int 
process_events(ETp etp, et_event **pe, int start, int stop)
{
  int handle1, i, buflen, len, tmp, status=0;
  unsigned int *buffer, *ptr;
  int event, type;
  size_t size;

  /* evio file output */
  for (i=start; i <= stop; i++)
  {
    len = pe[i]->length>>2;
    /*printf("Got event of length %d bytes.\n", len);*/

	etp->object->nlongs += len;
    etp->nlongs += len;
    etp->object->nevents++;
    etp->nevents++;

    et_event_getlength(pe[i], &size);
    len = size;
    ptr = (unsigned int *)pe[i]->pdata;
	
    /* skip evio record header, evLinkBnk() etc functions assumes there is no record header */
    ptr += 8;
    len -= 32;
	

    /**********************/
    /* process event here */
    /**********************/


    /*printf("Processing event num=%d len=%d\n",etp->nevents,len);*/

    event = etp->nevents;
    type = pe[i]->control[0];
	/*printf("L3: calling helicity, type=%d, event=%d\n",type,event);*/
    helicity(ptr, type);
	/*printf("L3: helicity is done\n");*/


    /**********************/
    /**********************/
    /**********************/


	/*
    status = evWrite(etp->fd, ptr);
	*/
    if(status!=0) {printf("evWrite returns %d\n",status);return(status);}
  }

  /* if error writing coda file */ 
  if (status != 0)
  {
    return(status);
  }   
  
  return(CODA_OK);
}


int 
events_loop(ETp etp, int flag)
{
  const int prestartEvent=EV_ER_PRESTART, endEvent=EV_ER_END, true=1, false=0;
  int status1, status2;
  int nevents=0, i, ii, status, len, done=false, start, stop;
  et_event *pe[ET_EVENT_ARRAY_SIZE];
  struct timespec waitfor;
  struct tms tms_start, tms_end;
  clock_t start_ticks, stop_ticks;
  
  waitfor.tv_sec  = 0;
  waitfor.tv_nsec = 10000000L; /* .010 sec */
  
  if (!et_alive(et_sys))
  {
    printf("events_loop: not attached to ET system\n");
    et_reinit = 1;
    return(0);
  }

  if( (start_ticks = times(&tms_start)) == -1)
  {
    printf("events_loop: ERROR getting start time\n");
  }

  do
  {
    status1 = et_events_get(et_sys, et_attach, pe, ET_SLEEP,
                            NULL, ET_EVENT_ARRAY_SIZE, &nevents);
    /*printf("Got %d events from ET\n",nevents);fflush(stdout);*/

    /* if no events or error ... */
    if ((nevents < 1) || (status1 != ET_OK))
    {
      /* if status1 == ET_ERROR_EMPTY or ET_ERROR_BUSY, no reinit is necessary */
      
      /* will wake up with ET_ERROR_WAKEUP only in threaded code */
      if (status1 == ET_ERROR_WAKEUP)
      {
        printf("events_loop: forcing writer to quit read, status = ET_ERROR_WAKEUP\n");
      }
      else if (status1 == ET_ERROR_DEAD)
      {
        printf("events_loop: forcing writer to quit read, status = ET_ERROR_DEAD\n");
        et_reinit = 1;
      }
      else if (status1 == ET_ERROR)
      {
        printf("events_loop: error in et_events_get, status = ET_ERROR \n");
        et_reinit = 1;
      }
      done = true;
    }
    else    /* if we got events ... */
    {
      /* by default (no control events) write everything */
      start = 0;
      stop  = nevents - 1;
      
      /* if we got control event(s) ... */
      if (gotControlEvent(pe, nevents))
      {
        /* scan for prestart and end events */
        for (i=0; i<nevents; i++)
        {
	      if (pe[i]->control[0] == prestartEvent)
          {
	        printf("Got Prestart Event!!\n");
			mbytes_in_current_run = 0;
            nevents_in_current_run = 0;
	        /* look for first prestart */
	        if (PrestartCount == 0)
            {
	          /* ignore events before first prestart */
	          start = i;
	          if (i != 0)
              {
	            printf("events_loop: ignoring %d events before prestart\n",i);
	          }
	        }
            PrestartCount++;
	      }
	      else if (pe[i]->control[0] == endEvent)
          {
            etp->nend--;
	        /* ignore events after last end event & quit */
	        if (etp->nend <= 0)
            {
	          stop = i;
	          done = true;
	        }
            printf("Got End event, Need %d more\n",etp->nend);
	      }
        }
      } 
      
      /* write events to output (ignore events before first prestart) */
      if (PrestartCount != 0)
      {
        if (process_events(etp, pe, start, stop) != 0)
        {
	      /* error writing coda file so quit */
	      printf("ERROR: Error writing events... Cancel ET read loop!\n");
          done = true;
        }
      }

      /* put et events back into system */
      status2 = et_events_put(et_sys, et_attach, pe, nevents);            
      if (status2 != ET_OK)
      {
	    printf("events_loop: error in et_events_put, status = %i \n",status2);
        et_reinit = 1;
        done = true;
      }
      /*printf("Put %d events to ET\n",nevents);fflush(stdout);*/
    }

    if( (stop_ticks = times(&tms_end)) == -1)
    {
      printf("events_loop: ERROR getting stop time\n");
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
	    /*printf("events_loop: WARN: L3 is backed up! This may be causing system deadtime\n");*/
	    done = true;
      }
    }

  } while(done == false);
  
  if(etp->nend <= 0)
  { 
    printf("events_loop: return(0)\n");
    return(0);
  }
  else
  {
    /*printf("events_loop: return(1)\n");*/
    return(1);
  }
}



/* do we need some sleep in do-while loop ? we are eating whole cpu ... */
void *
l3_process_thread(objClass object)
{
  ETp etp;
  int ix, status = 0;

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &ix);
  
  etp = (ETp) object->privated;
  force_exit = 0;
  do
  {
    status = events_loop(etp, force_exit);

    /*usleep(1000);*/ /* sleep for N microsec */

  } while (status);

  printf("l3_process_thread loop ended (status=%d)\n",status);

  pthread_exit(0);
}


/*********************************************************/
/*********************************************************/
/*********************************************************/


int
codaInit(char *conf)
{
  printf("coda_l3: codaInit() reached\n");fflush(stdout);

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
  /*
  objClass object = localobject;
  ETp etp = (ETp) object->privated;
  */
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;

  int  ix;  
  int  listArgc;
  char listArgv[LISTARGV1][LISTARGV2];

  static char tmp[1000];
  static char tmp2[1000];

  MYSQL *dbsock;
  char tmpp[1000];

  printf("codaDownload reached, etp=0x%08x\n",etp);fflush(stdout);

  etp->object = object;


  /*****************************/
  /*****************************/
  /* FROM CLASS (former conf1) */

  strcpy(configname,conf); /* Sergey: save CODA configuration name */
  printf("coda_l3: configname = >%s<\n",configname);fflush(stdout);

  UDP_start();

  tcpState = DA_DOWNLOADING;
  if(codaUpdateStatus("downloading") != L3_OK) return(L3_ERROR);



  /*****************************/
  /*****************************/
  /*****************************/

  printf("INFO: Downloading configuration '%s'\n", configname);




  /***************************************************/
  /* extract all necessary information from database */

  /* connect to database */
  dbsock = dbConnect(mysql_host, expid);

  /* Get the list of readout-lists from the database */
  sprintf(tmpp,"SELECT code FROM %s WHERE name='%s'",configname,object->name);
  if(dbGetStr(dbsock, tmpp, tmp)==L3_ERROR) return(L3_ERROR);
  printf("++++++======>%s<\n",tmp);

  /* disconnect from database */
  dbDisconnect(dbsock);





  /* Decode configuration string */

  listArgc = 0;
  if(!((strcmp (tmp, "{}") == 0)||(strcmp (tmp, "") == 0)))
  {
    if(listSplit1(tmp, 1, &listArgc, listArgv)) return(L3_ERROR);
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
    if(l3_et_initialize() != L3_OK)
    {
      printf("ERROR: l3 download: cannot initalize ET system\n");
      return(L3_ERROR);
    }
  }




  printf("coda_l3: downloaded !!!\n");

  tcpState = DA_DOWNLOADED;
  if(codaUpdateStatus("downloaded") != L3_OK) return(L3_ERROR);

  return(L3_OK);
}


int
codaPrestart()
{
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;

  MYSQL *dbsock;
  char tmpp[1000];

  printf("INFO: Prestarting\n");

  /*
  dbsock = dbConnect(mysql_host,  expid);

  sprintf(tmpp,"SELECT runNumber FROM sessions WHERE name='%s'",session);
  if(dbGetInt(dbsock,tmpp,&(object->runNumber))==L3_ERROR) return(L3_ERROR);

  dbDisconnect(dbsock);
  */

  printf("INFO: prestarting,run %d, type %d\n",
    object->runNumber, object->runType);

  PrestartCount = 0;
  object->nevents = 0;
  object->nlongs = 0;

  etp->nend = 1;


  /* starting main thread here */

  etp->process_thread = 0; /*sergey: will check it*/
  printf("starting write thread 1 ..\n");fflush(stdout);
  pthread_attr_t detached_attr;
  pthread_attr_init(&detached_attr);
  pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setscope(&detached_attr, PTHREAD_SCOPE_SYSTEM);
  pthread_create( &etp->process_thread, &detached_attr, (void *(*)(void *)) l3_process_thread, (void *) object);


  /*
  erDaqCmd("open");
  */



  tcpState = DA_PAUSED;
  codaUpdateStatus("paused");

  return(L3_OK);
}

int
codaGo()
{
  tcpState = DA_ACTIVE;
  codaUpdateStatus("active");
  return(L3_OK);
}

int
codaEnd()
{
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;
  void *status;
  printf("waiting for write thread 1 ..\n");fflush(stdout);

  et_wakeup_attachment(et_sys, et_attach);
  printf("waiting for write thread 2 .. etp->process_thread=0x%08x\n",etp->process_thread);fflush(stdout);

  /* if thread was started, cancel it */
  if(etp->process_thread > 0)
  {
	/*
    printf("update_database called ..\n");
	update_database(etp);
    printf(".. update_database done\n");
	*/
    /*pthread_cancel(etp->process_thread);*/

    printf("waiting for write thread 3 ..\n");fflush(stdout);

    /* set force_exit=1 to tell writer that it has to exit 
    force_exit = 1; - not used in CODA_write_event yet, may not need it ...
    */

    pthread_join(etp->process_thread, &status);

    etp->process_thread = 0; /*sergey: will check it*/
    printf("write thread is done\n");fflush(stdout);
  }


  /* restore force_exit - just in case */
  force_exit = 0;



  tcpState = DA_DOWNLOADED;
  codaUpdateStatus("downloaded");
  return(L3_OK);  
}

int
codaPause()
{
  tcpState = DA_PAUSED;
  codaUpdateStatus("paused");
  return(L3_OK);
}

int
codaExit()
{
  tcpState = DA_CONFIGURED;
  codaUpdateStatus("configured");
  /*
  UDP_reset();
  */
  return(L3_OK);
}


/****************/
/* main program */
/****************/

void
main (int argc, char **argv)
{
  CODA_Init(argc, argv);

  printf("\n\n\n l3Constructor called ..\n\n");

  l3Constructor();

  sleep(3);
  printf("\n\n\n l3Constructor started\n\n");

  /* CODA_Service ("ROC"); */
  CODA_Execute ();

  l3Destructor(); /* never called ... */
}

#endif
