
/* coda_et.c - CODA wrapper for et_start */

#include <stdio.h>


#if defined(Linux_armv7l)

int
main()
{
  printf("coda_et is dummy for ARM etc\n");
}

#else

/* INCLUDES */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#ifdef Linux
#include <sys/prctl.h>
#endif
#if defined __sun||LINUX
#include <dlfcn.h>
#endif
#ifdef Linux
#include <unistd.h> /* for usleep() */
#endif

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


typedef int (*IFUNCPTR) ();

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



static int force_exit = 0;
static int PrestartCount = 0;
/*static*/extern objClass localobject;
extern char configname[128]; /* coda_component.c */
extern char *mysql_host; /* coda_component.c */
extern char *expid; /* coda_component.c */
extern char *session; /* coda_component.c */

/*defined in et.h
#define ET_ERROR 1
#define ET_OK 0
*/

int listSplit1(char *list, int flag,
           int *argc, char argv[LISTARGV1][LISTARGV2]);



/* Define time limit for staying in write loop (seconds) */
#define ET_WRITE_LOOP_TIMEOUT   10



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



/* our version of 'et_start' */
int
etStart()
{
  localobject->privated = (void *) &ETP;
  bzero ((char *) &ETP,sizeof(ETP));

  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;

  size_t hostlen = ET_FILENAME_LENGTH - 1;
  char *ch;

  int status;
  MYSQL *dbsock;
  char tmp[1000], tmpp[1000];

  int  ix;  
  int  listArgc;
  char listArgv[LISTARGV1][LISTARGV2];

  /************************************/
  /* default configuration parameters */
  /************************************/
  etp->nevents = 4000;      /* total number of events */
  etp->event_size = 500000; /* size of event in bytes */

  etp->serverPort = 11111;  /* server port number, unique for every ET system on the same machine */
                            /* will NOT use that value, will get free port and put it into database */

  etp->udpPort = 0;
  etp->deleteFile = 1;
  etp->ngroups = 1;    
  etp->noDelay = 0;
  etp->maxNumStations = 0;
  etp->sendBufSize = 0;
  etp->recvBufSize = 0;
  etp->et_verbose = 1/*ET_DEBUG_NONE*/;


  /* Use default multicast address */
  memset(etp->mcastAddr, 0, ET_IPADDRSTRLEN);
  strcpy(etp->mcastAddr, ET_MULTICAST_ADDR);

  /* ET file name uses session name */
  strncpy(etp->et_filename, session, ET_FILENAME_LENGTH-1);
  printf("et_start: session name '%s' will be used to construct ET file name\n", etp->et_filename);

  /* check length of name */
  if ( (strlen(etp->et_filename) + 12) >=  ET_FILENAME_LENGTH)
  {
    fprintf(stderr, "ET file name is too long\n");
    exit(-1);
  }

  /*sergey: sprintf(etp->et_name, "/tmp/et_sys_%s", etp->et_filename);*/
  sprintf(etp->et_name, "/et/%s", etp->et_filename);

  printf("et_start: ET name '%s'\n", etp->et_name);


  if (etp->et_verbose)
  {
    printf("et_start: ET version %d; %d - %d byte-sized events\n", ET_VERSION, etp->nevents, etp->event_size);
  }

  if (etp->deleteFile)
  {
    remove(etp->et_name);
  }


  

  /**********************************************/
  /* get configuration parameters from database */
  /**********************************************/

  /* connect to database */
  dbsock = dbConnect(mysql_host, expid);





  /* we do not have config name yet, will get it in Download(), so get the config name
  from the database table 'session' - it means RUNCONTROL HAVE TO BE STARTED AND 'Configure' EXECUTED BY NOW !!! */
  printf("coda_et: trying to get 'config' from 'sessions' table for session >%s<\n",session);
  sprintf(tmp,"SELECT config FROM sessions WHERE name='%s'",session);
  if(dbGetStr(dbsock, tmp, tmpp)==ET_ERROR)
  {
    printf("coda_et: (1) cannot get 'config' from 'sessions' table for session >%s<\n",session);
  }
  else if(strlen(tmpp)==0)
  {
    printf("coda_et: (2) cannot get 'config' from 'sessions' table for session >%s<\n",session);
  }
  else
  {
    strcpy(configname,tmpp);
    printf("Got 'config' from 'session' table ======>%s<\n",configname);

    /*nevents and event_size used to be in _option table, now it is 'code' field in config table, see below
    sprintf(tmp,"SELECT value FROM %s_option WHERE name='ET_nevents'",configname);
    if(dbGetInt(dbsock, tmp, &etp->nevents)==ET_ERROR)
    {
      etp->nevents = 1000;
      printf("WARN: database does not have ET_nevents, will use default %d\n",etp->nevents);
    }

    sprintf(tmp,"SELECT value FROM %s_option WHERE name='ET_event_size'",configname);
    if(dbGetInt(dbsock, tmp, &etp->event_size)==ET_ERROR)
    {
      etp->event_size = 50000;
      printf("WARN: database does not have ET_event_size, will use default %d\n",etp->event_size);
    }
    */

    /* Get nevents and event_size from 'code' field in config table */
    sprintf(tmpp,"SELECT code FROM %s WHERE name='%s'",configname,object->name);
    if(dbGetStr(dbsock, tmpp, tmp)==ET_ERROR) return(ET_ERROR);

    /* Decode 'code' string extracting the number of events and event size */
    listArgc = 0;
    if(!((strcmp (tmp, "{}") == 0)||(strcmp (tmp, "") == 0)))
    {
      if(listSplit1(tmp, 1, &listArgc, listArgv)) return(ET_ERROR);
      for(ix=0; ix<listArgc; ix++) printf("code [%1d] >%s<\n",ix,listArgv[ix]);
      if(listArgc>0) etp->nevents = atoi(listArgv[0]);
      if(listArgc>1) etp->event_size = atoi(listArgv[1]);
    }
    else
    {
      printf("Nothing in 'code' field >%s<, will use default values\n",tmp);
      etp->nevents = 400;
      etp->event_size = 200000;
    }
  }

  /* obtain our hostname */
  status = gethostname(etp->host,hostlen);  
  if(status!=0)
  {
    printf("error in gethostname - exit\n");
    exit(-1);
  }
  else
  {
    printf("Hostname (full) >%s<\n",etp->host);
    ch = strstr ( etp->host, "." );
    if(ch != NULL) *ch = '\0';
    printf("Hostname (to be used) >%s<\n",etp->host);
  }


  /*etp->serverPort = codaFindFreeTcpPort(); - SERGEY: WILL DO IT IN FUTURE !!! */
  printf("Will use port %d\n",etp->serverPort);

  /* problem: port was released after check, somebody may grab it before we et gets it ... */

  sprintf(tmp,"UPDATE process SET host='%s', port='%d' WHERE name='%s'",etp->host,etp->serverPort,object->name);
  printf("DB update: >%s<\n",tmp);
  if(mysql_query(dbsock, tmp) != 0)
  {
    printf("DB update ERROR - exit\n");
    exit(0);
  }



  /* disconnect from database */
  dbDisconnect(dbsock);




  printf("\nSet following parameters:\n");
  printf("   et_filename >%s<\n",etp->et_name);
  printf("   nevents = %d\n",etp->nevents);
  printf("   event_size = %d\n",etp->event_size);
  printf("   hostname >%s<\n",etp->host);
  printf("   serverPort = %d\n",etp->serverPort);
  printf("   udpPort = %d\n",etp->udpPort);
  printf("   deleteFile = %d\n",etp->deleteFile);
  printf("   noDelay = %d\n",etp->noDelay);
  printf("   ngroups = %d\n",etp->ngroups);
  printf("   maxNumStations = %d\n",etp->maxNumStations);
  printf("   sendBufSize = %d\n",etp->sendBufSize);
  printf("   recvBufSize = %d\n",etp->recvBufSize);
  printf("   et_verbose = %d\n",etp->et_verbose);
  printf("\n");



  /********************************/
  /* set configuration parameters */
  /********************************/
  
  if (et_system_config_init(&etp->config) == ET_ERROR)
  {
    printf("et_start: no more memory\n");
    exit(1);
  }

  /* divide events into equal groups and any leftovers into another group */
  if (etp->ngroups > 1) {
      int i, addgroup=0, *groups, totalNE=0;
        
      int n = etp->nevents / etp->ngroups;
      int r = etp->nevents % etp->ngroups;
      if (r > 0) {
          addgroup = 1;
      }

      groups = calloc(etp->ngroups+addgroup, sizeof(int));
      if (groups == NULL) {
          printf("et_start: out of memory\n");
          exit(1);
      }

      for (i=0; i < etp->ngroups; i++) {
          groups[i] = n;
          totalNE += n;
      }

      if (addgroup) {
          groups[etp->ngroups] = r;
          totalNE += r;
      }
      printf("Have %d total events divided\n",totalNE);
      et_system_config_setgroups(etp->config, groups, etp->ngroups + addgroup);

      free(groups);
  }
  
  /* total number of events */
  et_system_config_setevents(etp->config, etp->nevents);

  /* size of event in bytes */
  et_system_config_setsize(etp->config, etp->event_size);

  /* max # of stations */
  if (etp->maxNumStations > 1)
  {
      et_system_config_setstations(etp->config, etp->maxNumStations);
  }

  /* limit on # of stations = 20 */
  /* hard limit on # of processes = 110 */
  /* hard limit on # of attachments = 110 */
  /* max # of temporary (specially allocated mem) events = 300 */

  /* set TCP server port */
  if (etp->serverPort > 0) et_system_config_setserverport(etp->config, etp->serverPort);
  
  /* set UDP (broadcast/multicast) port */
  if (etp->udpPort > 0) et_system_config_setport(etp->config, etp->udpPort);

  /* set server's TCP parameters */
  et_system_config_settcp(etp->config, etp->recvBufSize, etp->sendBufSize, etp->noDelay);
  
  /* add multicast address to listen to  */
  if (strlen(etp->mcastAddr) > 7)
  {
    status = et_system_config_addmulticast(etp->config, etp->mcastAddr);
    if (status != ET_OK)
    {
        printf("et_start: bad multicast address argument\n");
        exit(1);
    }
  }

  /* Make sure filename is null-terminated string */
  if (et_system_config_setfile(etp->config, etp->et_name) == ET_ERROR)
  {
    printf("et_start: bad filename argument\n");
    exit(1);
  }
  



  
  sigfillset(&etp->sigblockset);
  status = pthread_sigmask(SIG_BLOCK, &etp->sigblockset, NULL);
  if (status != 0)
  {
    printf("et_start: pthread_sigmask failure\n");
    exit(1);
  }
  sigemptyset(&etp->sigwaitset);
  sigaddset(&etp->sigwaitset, SIGTERM);
  sigaddset(&etp->sigwaitset, SIGINT);
  

  /*************************/
  /*    start ET system    */
  /*************************/
  if (etp->et_verbose)
  {
    printf("et_start: starting ET system %s\n", etp->et_name);
  }

  if (et_system_start(&etp->id, etp->config) != ET_OK)
  {
    printf("et_start: error in starting ET system\n");
    exit(1);
  }


  et_system_setdebug(etp->id, etp->et_verbose);

  /* turn this thread into a signal handler */
  sigwait(&etp->sigwaitset, &etp->sig_num);
  

  
  printf("Interrupted by CONTROL-C or SIGTERM\n");
  printf("ET is exiting\n");
  et_system_close(etp->id);
  

  return(ET_OK);
}


int
etConstructor()
{
  int res;
  pthread_t id;
  pthread_attr_t attr;

  /*
  etStart();
goto a321;
  */

  pthread_attr_init(&attr); /* initialize attr with default attributes */
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  res = pthread_create(&id, &attr, (void *(*)(void *)) etStart, NULL);
  if(res!=0)
  {
    printf("ERROR: pthread_create(etStart) returned %d - exit\n",res);
    exit(-1);
  }

  tcpState = DA_BOOTED;
  if(codaUpdateStatus("booted") != ET_OK) return(ET_ERROR);

  tcpServer(localobject->name, mysql_host); /*start server to process non-coda commands sent by tcpClient*/

a321:
  ;
}


int
etDestructor()
{
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;

  printf("Closing ET system\n");
  et_system_close(etp->id);

}


/*********************************************************/
/*********************************************************/
/*********************************************************/


int
codaInit(char *conf)
{
  printf("coda_et: codaInit() reached\n");fflush(stdout);

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
  ET_priv *etp = (ET_priv *) object->privated;

  static char tmp[1000];
  static char tmp2[1000];

  MYSQL *dbsock;
  char tmpp[1000];

  
  etp->object = object;



  /*****************************/
  /*****************************/
  /* FROM CLASS (former conf1) */

  strcpy(configname,conf); /* Sergey: save CODA configuration name */

  UDP_start();

  tcpState = DA_DOWNLOADING;
  if(codaUpdateStatus("downloading") != ET_OK) return(ET_ERROR);



  /*****************************/
  /*****************************/
  /*****************************/

  printf("INFO: Downloading configuration '%s'\n", configname);




  /***************************************************/
  /* extract all necessary information from database */

  /* connect to database */
  dbsock = dbConnect(mysql_host, expid);

  /* disconnect from database */
  dbDisconnect(dbsock);

  /* If we need to initialize, reinitialize, or
   * if et_alive fails on Linux, then initialize.
  if( (et_init == 0)   ||
      (et_reinit == 1) ||
      ((!et_alive(et_sys)) && (et_locality == ET_LOCAL_NOSHARE))
     )
  {
    et_start();
  }
  */


  printf("coda_et: downloaded !!!\n");

  tcpState = DA_DOWNLOADED;
  if(codaUpdateStatus("downloaded") != ET_OK) return(ET_ERROR);

  return(ET_OK);
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
  if(dbGetInt(dbsock,tmpp,&(object->runNumber))==ET_ERROR) return(ET_ERROR);

  dbDisconnect(dbsock);
  */

  printf("INFO: prestarting,run %d, type %d\n",
    object->runNumber, object->runType);

  PrestartCount = 0;
  object->nevents = 0;
  object->nlongs = 0;

  tcpState = DA_PAUSED;
  codaUpdateStatus("paused");

  return(ET_OK);
}

int
codaGo()
{
  tcpState = DA_ACTIVE;
  codaUpdateStatus("active");
  return(ET_OK);
}

int
codaEnd()
{
  tcpState = DA_DOWNLOADED;
  codaUpdateStatus("downloaded");
  return(ET_OK);  
}

int
codaPause()
{
  tcpState = DA_PAUSED;
  codaUpdateStatus("paused");
  return(ET_OK);
}

int
codaExit()
{
  tcpState = DA_CONFIGURED;
  codaUpdateStatus("configured");
  /*
  UDP_reset();
  */
  return(ET_OK);
}


/****************/
/* main program */
/****************/

void
main (int argc, char **argv)
{
  CODA_Init(argc, argv);

  etConstructor();

  sleep(3);
  printf("\n\n\n etConstructor started\n\n");

  /* CODA_Service ("ROC"); */
  CODA_Execute ();

  etDestructor(); /* never called ... */
}

#endif
