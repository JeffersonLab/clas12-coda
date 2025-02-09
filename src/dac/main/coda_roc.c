
/* roc_component.c */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>

#ifdef Linux
#include <unistd.h>
#include <sys/prctl.h>
#endif

#ifdef Linux_vme
#include "jvme.h"
#endif

#define MYCLOCK NANOMICRO


static pthread_t iTaskROL;
static pthread_t iTaskPROC;
static pthread_t iTaskNET;
static pthread_attr_t detached_attr;


#ifdef Linux
int tcpServer(char *name, char *mysqlhost);
#endif


#include "rc.h"
#include "rolInt.h"
#include "da.h"
#include "circbuf.h"
#include "bigbuf.h"

#include "roc_process.h"
#include "roc_network.h"


static unsigned int tloop1=0;
static unsigned int tloop2=0;
static unsigned int tloop3=0;
static unsigned int tloop4=0;
static unsigned int tloop5=0;
static unsigned int tloop6=0;


#define CODA_ERROR 1
#define CODA_OK 0



typedef struct roc_private_store *rocParam;
typedef struct roc_private_store
{
  ROLPARAMS *rolPs[2];       /* pointers to readout lists */
  int        async_roc_flag; /* if =1 then roc communication with eb inhibited */
  /*int        recNb; use global rocp_ recNb*/         /* number of records sent by this ROC */
  int        active;         /* set to 1 if we are supposed to take data */
  int        state;          /* current state as a numeric value */
  /*int        last_event; use global 'last_event_check'*/     /* number of last event sent on network */
  /*int        primefd; use global rocp_primefd*/
  int        output_switch;
} roc_priv;



static int nrols;
static ROLPARAMS *rolPs[2]; /* will be allocated once in constructor */

int rocMask = 0;

/*static*/ int64_t object_nlongs; /* instead of object->nlongs*/
/*static*/ int clear_to_send = 0;
/*static*/ int last_event_check = 0;
/*static*/ int rocp_primefd;
/*static*/ int rocp_recNb;

static /*time_t*/hrtime_t currenttime;
static /*time_t*/hrtime_t timeout;



/* Global Symbols */
int token_interval;

/* for the time profiling */
static int icycle;
static int cycle;
static int icycle3;
static int cycle3;

#ifndef Darwin
static hrtime_t time3;
#endif


static char confFile[256];

extern char *mysql_host; /* coda_component.c */
extern char *expid; /* coda_component.c */
extern char *session; /* coda_component.c */

#include "libdb.h"


/* minimum memory space left after big buffers and event buffers allocation */
#define MIN_MEM_LEFT (1024 * 1024 * 3)

/* the number of output buffers */
/* buffer size defined in circbuf.h */
#ifdef Linux_nios2
#define NUM_SEND_BUFS        4
#else
#define NUM_SEND_BUFS        16
#endif

static int print_output_buffers_are_full = 1; /* enable warning message */
/* was reenabled in LINK_sized_write, removed temporarily */

/*static*/ int tsendBufSize = 0; /* total buffer size; can be <= SEND_BUF_SIZE */

/*static*/ unsigned int g_max_ev_in_buf = 32;
/*static*/ unsigned int g_events_in_buffer = 0;

/* socket number */
static int socketnum = 0;

/* ROC id */
/*static*/ int this_roc_id = 0;

/* two pointers used to store ROL2 output to 'big' buffers */
/*static*/ uint32_t *dabufp; /* non-static to be used by ROL1 */
/*static*/ uint32_t dataInBuf = BBHEAD_BYTES;


unsigned int dabufp_usermembase; /* non-static to be used by ROL1 */
unsigned int dabufp_physmembase; /* non-static to be used by ROL1 */





/* big buffers to be used as VME DMA destination */
/*static*/ BIGBUF *gbigDMA = NULL;

/* big buffers to be used between 'coda_proc' and 'coda_net' */
static BIGBUF *gbigBUF = NULL;

/* net_thread structure, buffers from it to be used by 'coda_proc', the rest by 'coda_net' */
static BIGNET bigproc;

/* net_thread structure, contains pointer to the gbigBUF - to be used
 between 'coda_proc' and 'coda_net' */
static BIGNET bignet;


extern char configname[128]; /* coda_component.c */

/*static*/extern objClass localobject;
void output_proc_network(int bla/*struct alist *output*/);


int LINK_establish(char *host, int port);
int LINK_close(int socket);

void proc_thread(BIGNET *bigprocptr);
void net_thread(BIGNET *bignetptr);

void rols_loop();
static int rols_loop_exit = 0;





/* some access functions */

char *
dacGetExpid()
{
  return(expid);
}




/* semaphore to synchronize CODA transitions and output_proc_network;
second parameter in semTake() is in 'ticks', we'll wait 5 seconds */

static pthread_mutex_t transition_lock;
#define TRANSITION_LOCK pthread_mutex_lock(&transition_lock)
#define TRANSITION_UNLOCK pthread_mutex_unlock(&transition_lock)


/*static*/ pthread_mutex_t sendbuffer_lock;
#define SENDBUFFER_LOCK pthread_mutex_lock(&sendbuffer_lock)
#define SENDBUFFER_UNLOCK pthread_mutex_unlock(&sendbuffer_lock)


/* local functions */
int codaDownload(char *confname);
int codaEnd();
int codaPrestart();
int codaGo();


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


int
rocId()
{
  return(this_roc_id);
}

int
rocIdprint()
{
  printf("rocId=%d\n",this_roc_id);
  return(this_roc_id);
}













/****************************************************************************/
/************************************ ROC ***********************************/

int
roc_constructor()
{
  int ix, res;
  unsigned int maxAvailBytes = 0;
  unsigned int maxNeededBytes = 0;
  rocParam rocp;
  char tmp[400];

  printf("roc_constructor: localobject=0x%08x\n",localobject);

  rocp = localobject->privated = (void *) malloc(sizeof (roc_priv));
  if(rocp == NULL)
  {
    printf("malloc error - return\n");
    return(CODA_ERROR);
  }
  memset((char *) localobject->privated, 0, sizeof (roc_priv));

  memset((char *) &bigproc, 0, sizeof(BIGNET));
  memset((char *) &bignet, 0, sizeof(BIGNET));


#ifdef Linux_vme
  printf("\n\n ======= clear dma memory ===================================\n");
  bb_dma_free();

  printf("\n\n coda_roc: ======= Close the default VME windows =========\n\n");
  /*vmeCloseA32Slave();*/
  vmeCloseDefaultWindows();

  printf("\n\n coda_roc: ======= Open the default VME windows =========\n\n");
  vmeOpenDefaultWindows();
  /*sergey: NO MORE SFIs         vmeOpenSlaveA32(0x18000000,0x00400000);*/ /* Open Slave Window for SFI Initiated Block transfers */
#endif

  /*************************/
  /* allocate buffer space */

  /* memory size we need */
  maxNeededBytes =
    SEND_BUF_SIZE*NUM_SEND_BUFS*2 /* multiply by 2 because need pool for proc and net */
    + MIN_MEM_LEFT;
  printf("min=0x%08x (bufs 0x%08x x %d x 2, mem=0x%08x)\n",
    maxNeededBytes,SEND_BUF_SIZE,NUM_SEND_BUFS,MIN_MEM_LEFT);

  maxAvailBytes = SEND_BUF_SIZE * NUM_SEND_BUFS;
  tsendBufSize = SEND_BUF_SIZE;
  printf("INFO: wants=0x%08x, maxAvailBytes=0x%08x, create=0x%08x\n",
      maxNeededBytes, maxAvailBytes, tsendBufSize);

  /* create input 'big' buffer pools for 'proc' and 'net'; input to the 'net' will be
  used as output from 'proc' if both of them are running on host */



  /* DMA buffer */
#if defined(NEW_ROC) && defined(Linux_vme)
bla1
  gbigDMA = bb_new_rol1(1,NUM_SEND_BUFS,tsendBufSize);
  if(gbigDMA == NULL)
  {
    printf("ERROR in bb_new_rol1: Buffer allocation FAILED\n");
    return(CODA_ERROR);
  }
  else
  {
    printf("bb_new_rol1: big buffer allocated at 0x%08x\n",gbigDMA);
  }

#else

  gbigDMA = bb_new(1,NUM_SEND_BUFS,tsendBufSize);
  if(gbigDMA == NULL)
  {
    printf("ERROR in bb_new: Buffer allocation FAILED\n");
    return(CODA_ERROR);
  }
  else
  {
    printf("bb_new: bigproc buffer allocated at 0x%08x\n",gbigDMA);
  }
#endif

  /***********************/
  /* intermediate buffer */
  gbigBUF = bb_new(2,NUM_SEND_BUFS,tsendBufSize);
  if(gbigBUF == NULL)
  {
    printf("ERROR in bb_new: gbigBUF allocation FAILED\n");
    return(CODA_ERROR);
  }
  else
  {
    printf("bb_new: gbigBUF allocated at 0x%08x\n",gbigBUF);
  }



  printf("=== %d %d\n",tcpState,rocp->state);

  if(codaUpdateStatus("booted") != CODA_OK) return(CODA_ERROR);

  /* Sergey */
  tcpState = rocp->state = DA_BOOTED;

  for(ix=0; ix<2; ix++)
  {
    rolPs[ix] = (ROLPARAMS *) malloc(sizeof(ROLPARAMS));
    if(rolPs[ix]==NULL)
    {
      printf("ERROR: cannot allocate rolPs[ix] !!!\n");
      return(CODA_ERROR);
    }
  }

  pthread_mutex_init(&transition_lock, NULL);
  pthread_mutex_init(&sendbuffer_lock, NULL);

  tcpServer(localobject->name, mysql_host); /*start server to process non-coda commands sent by tcpClient*/

  return(CODA_OK);
}

int
roc_destructor()
{
  ROLPARAMS  *rolP;
  int         ix, res;
  rocParam    rocp;

  printf("================== roc_destructor reached\n");

  if(localobject == NULL) return(CODA_OK);
  rocp = (rocParam) localobject->privated;

  if(rocp)
  {
    /* delete ROL1, free up memory etc... */
    if(rocp->rolPs[0] != NULL)
    {
      /* Must have called download once before */
      rolP = rocp->rolPs[0];

      /* ROL1: Delete buffer pools */
      if(rolP->inited)
      {
        rolP->daproc = DA_FREE_PROC;
        (*(rolP->rol_code)) (rolP);
      }

      /* unload ROL1 module if possible */
      if((rolP->id != 0) && (rolP->nounload == 0))
      {
        printf("Unloading old ROL object module 0x%08x\n",rolP->id);

		res = codaUnloadROL(rolP);
        if(res) return(CODA_ERROR);
      }
      rocp->rolPs[0] = NULL;
    }
    if(rocp) {free((char *)rocp);rocp=NULL;}
  }

  for(ix=0; ix<2; ix++) free(rolPs[ix]);

  pthread_mutex_unlock(&transition_lock);
  pthread_mutex_destroy(&transition_lock);

  pthread_mutex_unlock(&sendbuffer_lock);
  pthread_mutex_destroy(&sendbuffer_lock);

  return(CODA_OK);
}

int
rocCloseLink()
{
  printf("rocCloselink reached\n");


  /* do it on exit from net_thread */
  /* shutdown socket connection */
  /*socketnum = LINK_close(socketnum);*/

  /*
  if(socketnum != 0)
  {
    if(shutdown(socketnum, 2)==0)
    {
      printf("roc_closelink: socket #%d connection closed\n",socketnum);
      close(socketnum);
      socketnum = 0;
    }
    else
    {
      printf("roc_closelink: ERROR in socket #%d connection closing\n",
        socketnum);
    }
  }
  */


  return(CODA_OK);
}



int
rocCleanup()
{
  objClass object = localobject;

  rocParam rocp;

  printf("rocCleanup reached\n");
  rocp = (rocParam) object->privated;  
  bigproc.doclose = 0;
  bignet.doclose = 0;
  bigproc.failure = 0;
  bignet.failure = 0;
  rocp->active = 1;

  return(CODA_OK);
}


/* function called during 'Reset' transition; it will bring everything
   into initial state as it was before first 'Download' */

int
codaExit()
{
  objClass object = localobject;
  rocParam  rocp;
  ROLPARAMS *rolP;
  int       ix, ii;
  MYSQL *dbsock;
  char      tmpp[1000];

  rocp = (rocParam) object->privated;


  printf("\ncodaExit reached\n");
  printf("codaExit reached\n");
  printf("codaExit reached\n\n");


  /* mark ROC as async */
  /*sergey temporary
  rocp->async_roc_flag = 1;
  */

  codaEnd(); /*need it to 'end' hardware by doing what usually done in 'end' transinion*/

  rols_loop_exit = 1;
  ii = 6;
  while(rols_loop_exit)
  {
    printf("exit: wait for rols_loop to exit ..\n");
    sleep(1);
    ii --;
    if(ii<0) break;
  }

  rocCleanup();

  /*sergey: exit bb_write's if still active */
  if(nrols>1)
  {
    printf("\nCalls bb_cleanup 1\n");
    printf("Calls bb_cleanup 1\n");
    printf("Calls bb_cleanup 1\n\n");
    bb_cleanup(&bigproc.gbigin);
  }

  printf("\nCalls bb_cleanup 2\n");
  printf("Calls bb_cleanup 2\n");
  printf("Calls bb_cleanup 2\n\n");
  bb_cleanup(&bignet.gbigin);



  if(codaUpdateStatus("configured") != CODA_OK) return(CODA_ERROR);
  /*
  UDP_reset();
  */
  return(CODA_OK);
}


/*
 * This routine is called by the RC system to handle DOWNLOAD.
 * 
 * Put here the code to handle downloading the readout list and calling
 * the user download list.
 */

int
mytest1(char *confname)
{
  MYSQL *dbsock;
  MYSQL_RES *result;
  MYSQL_ROW row;
  char tmp[1000], tmpp[1000];
  int numRows;

  /* connect to database */
  dbsock = dbConnect(mysql_host, expid);
  printf("3122: dbsock=%d\n",dbsock);
  if(dbsock==NULL)
  {
    printf("cannot connect to the database 2 - exit\n");
    exit(0);
  }


  sprintf(tmpp,"SELECT id FROM runTypes WHERE name='%s'",confname);
  if(mysql_query(dbsock, tmpp) != 0)
  {
    printf("ERROR: cannot select id from runTypes\n");
    return(CODA_ERROR);
  }
  else
  {
    printf("id selected\n");
  }


  /* gets results from previous query */
  /* we assume that tableRes->numRows=1 */
  if( !(result = mysql_store_result(dbsock)) )
  {
    printf("ERROR in mysql_store_result()\n");
    return(CODA_ERROR);
  }
  else
  {
    numRows = mysql_num_rows(result);
    printf("nrow=%d\n",numRows);
    if(numRows == 1)
    {
      printf("=0=> >%s<\n",tmpp);
	  /*
      printf("=1=> 0x%08x\n",tableRes->queryData->data);
      printf("=1=> >%s< %d\n",*(tableRes->queryData->data),
        atoi(*(tableRes->queryData->data)));
      printf("=2=> %d %d %d >%s< >%s<\n",
                         tableRes->fieldData->field.type,
                         tableRes->fieldData->field.length,
                         tableRes->fieldData->field.flags,
                         tableRes->fieldData->field.name,
                         tableRes->fieldData->field.table);
      tableRes->fieldData = tableRes->fieldData->next;
      printf("=3=> %d %d %d >%s< >%s<\n",
                         tableRes->fieldData->field.type,
                         tableRes->fieldData->field.length,
                         tableRes->fieldData->field.flags,
                         tableRes->fieldData->field.name,
                         tableRes->fieldData->field.table);
	  */
    }
    else
    {
      printf("LINK_constructor_C: ERROR: unknown nrow=%d",numRows);
    }

    mysql_free_result(result);
  }

  /* disconnect from database */
  dbDisconnect(dbsock);

  return(0);
}

int listSplit1(char *list, int flag,
           int32_t *argc, char argv[LISTARGV1][LISTARGV2]);

/* for example: ../dac/tcpClient croctest10 download\(\"test_ts2\"\)" */

int
codaInit(char *confname)
{
  int res;

  TRANSITION_LOCK;
  if(codaUpdateStatus("configuring") != CODA_OK)
  {
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }

  res = UDP_start();
  if(res < 0) return(CODA_ERROR);

  if(codaUpdateStatus("configured") != CODA_OK)
  {
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }

  TRANSITION_UNLOCK;

  return(CODA_OK);
}

int
codaDownload(char *confname)
{
  objClass object = localobject;
  int numRows, ii, iii;
  MYSQL *dbsock;
  MYSQL_RES *result;
  MYSQL_ROW row;
  char tmp[1000], tmpp[1000];
  rocParam rocp;
  ROLPARAMS *rolP;
  int res, ix, state;

  TRANSITION_LOCK;
  if(codaUpdateStatus("downloading") != CODA_OK)
  {
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }

  /*********************************************************/
  /* get pointer to the structure allocated in constructor */
  /*********************************************************/
  rocp = (rocParam) object->privated;

  printf("download: object=0x%08x\n",object);
  printf("download: Downloading configuration >%s<\n",confname);
  fflush(stdout);

  /* set 'configname' */
  strcpy(configname,confname);
  printf("download: Downloading configuration >%s<\n",configname);
  fflush(stdout);

  getConfFile(configname, confFile, 255);

  res = UDP_start();
  if(res < 0) return(CODA_ERROR);

  /* update status again: UDP loop was just restarted so it does
	 not sends any messages at that point */
  if(codaUpdateStatus("downloading") != CODA_OK)
  {
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }

  /*********************************************************/
  /* before download, delete old lists, free up memory etc */
  /*********************************************************/
  if(rocp->rolPs[0] != NULL)
  {
    /* Must have called download once before */
    TRANSITION_UNLOCK;
    /* gracefully stop rol's loop */
    rols_loop_exit = 1;
    ii = 6;
    while(rols_loop_exit)
    {
      printf("download: wait for rols_loop to exit ..\n");
      sleep(1);
      ii --;
      if(ii<0) break;
    }

    if(ii<0)
	{
      printf("WARN: cannot exit rols_loop gracefully, will kill it\n");
      /* TODO: delete rols_loop thread */
      sleep(1);

      rols_loop_exit = 0; /* to let new ROLS_LOOP to start */
	}

    TRANSITION_LOCK;

    /* cleanup ROL1 */
    if((rolP = rocp->rolPs[0]) != NULL)
	{
      /* ROL1: Delete buffer pools */
      if(rolP->inited)
      {
        rolP->daproc = DA_FREE_PROC;
        (*(rolP->rol_code)) (rolP);
      }

      /* ROL1: unload module if possible */
      if((rolP->id != 0) && (rolP->nounload == 0))
      {
        printf("Unloading old ROL object module 0x%08x\n",rolP->id);
	    res = codaUnloadROL(rolP);
        if(res) return(CODA_ERROR);
      }

      rocp->rolPs[0] = NULL;
	}
  }

printf("31: >%s< >%s<\n",mysql_host, expid);fflush(stdout);


  /* connect to database */
  printf("mysql_host >%s<\n",mysql_host);fflush(stdout);
  dbsock = dbConnect(mysql_host, expid);
  printf("3123: dbsock=%d\n",dbsock);fflush(stdout);
  if(dbsock==NULL)
  {
    printf("cannot connect to the database 3 - exit\n");
    exit(0);
  }


  /******************************************************/
  /* get the run type number and save it somewhere safe */
  /******************************************************/
  sprintf(tmpp,"SELECT id FROM runTypes WHERE name='%s'",confname);
printf("3123: tmpp>%s<\n",tmpp);fflush(stdout);
  if(dbGetInt(dbsock, tmpp, &(object->runType))==CODA_ERROR)
  {
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }
  printf("====1==== !!! object->runType=%d\n",object->runType);


  /*******************************************************************/
  /* get the token Interval (ROC record size) from the configuration */
  /* options table of the database                                   */
  /*******************************************************************/
  token_interval = 60;
  sprintf(tmpp,"SELECT value FROM %s_option WHERE name='tokenInterval'",
    confname);
  if(dbGetInt(dbsock, tmpp, &token_interval)==CODA_ERROR)
  {
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }


  if(token_interval < 60) token_interval = 60;
  bigproc.token_interval = token_interval;
  bignet.token_interval = token_interval;
  printf("token_interval=%d\n",token_interval);
  printf("object->name >%s<\n",object->name);


  /*************************************************/
  /* check that we should be in this configuration */
  /*************************************************/
  sprintf(tmpp,"SELECT name FROM %s WHERE name='%s'",confname,object->name);
  if(mysql_query(dbsock, tmpp) != 0)
  {
    printf("ERROR: cannot select name from %s\n",confname);
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }
  else
  {
    printf("name selected\n");
  }
  /* gets results from previous query */
  if( !(result = mysql_store_result(dbsock)) )
  {
    printf("download: ERROR in mysql_store_result()\n");
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }
  else
  {
    numRows = mysql_num_rows(result);
    printf("nrow=%d\n",numRows);

    if(numRows == 1)
	{
      row = mysql_fetch_row(result);
      printf("download: name >%s<\n",row[0]);
      strcpy(tmpp, row[0]);
      if((strcmp (tmpp, "{}") == 0)||(strcmp (tmpp, "") == 0))
      {
        printf("ERROR: This component is not used in run type %s\n",
          confname);
        TRANSITION_UNLOCK;
        return(CODA_ERROR);
      }
    }
    else
    {
      printf("download: ERROR: unknown nrow=%d",numRows);
      TRANSITION_UNLOCK;
      return(CODA_ERROR);
    }

    mysql_free_result(result);
  }


  /***************************************************/
  /* get the list of readout-lists from the database */
  /***************************************************/  
  sprintf(tmpp,"SELECT code FROM %s WHERE name='%s'",confname,object->name);
  if(mysql_query(dbsock, tmpp) != 0)
  {
    printf("ERROR: cannot select code from %s\n",confname);
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }
  else
  {
    printf("code selected\n");
  }



  /* gets results from previous query */
  if( !(result = mysql_store_result(dbsock)) )
  {
    printf("download: ERROR in mysql_store_result()\n");
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }
  else
  {
    numRows = mysql_num_rows(result);
    printf("nrow=%d\n",numRows);
    if(numRows == 1)
    {
      row = mysql_fetch_row(result);
      printf("download: code >%s<\n",row[0]);
	  
      strcpy(tmpp, row[0]);
      if((strcmp (tmpp, "{}") == 0)||(strcmp (tmpp, "") == 0))
      {
        printf("ERROR: This component is not used in run type %s\n",
          confname);
        TRANSITION_UNLOCK;
        return(CODA_ERROR);
      }
    }
    else
    {
      printf("download: ERROR: unknown nrow=%d",numRows);
      TRANSITION_UNLOCK;
      return(CODA_ERROR);
    }

    mysql_free_result(result);
  }


dbDisconnect(dbsock);



  /********************************************/
  /* decode configuration string and download */
  /********************************************/
  strcpy(tmp, tmpp);
  if(!((strcmp (tmp, "{}") == 0)||(strcmp (tmp, "") == 0)))
  {
    int ix, jx;
    int listArgc;
    char listArgv[LISTARGV1][LISTARGV2];
    int tmpArgc;
    char tmpArgv[LISTARGV1][LISTARGV2];
    char name[20];
	/**
    ROL_MEM_PART **last_output;
	**/



    if(listSplit1(tmp, 1, &listArgc, listArgv))
    {
      TRANSITION_UNLOCK;
      return(CODA_ERROR);
    }

    /* the number of ROLs cannot exceed 2 */
    if(listArgc>2)
	{
      printf("roc_component ERROR: listArgc=%d, set it to 2\n",listArgc);
      listArgc = 2;
	}
    nrols = listArgc;

    for(ix=0; ix<nrols; ix++) printf("nrols [%1d] >%s<\n",ix,listArgv[ix]);

printf("codaDownload: listArgc=%d listArgv >%s< >%s<\n",listArgc,listArgv[0],listArgv[1]);
if(listArgc<=0)
{
  printf("ERROR: cannot download readout list(s) - exit\n");
  exit(0);
}

    /* set ROCid */
    this_roc_id = object->codaid;
    bigproc.rocid = this_roc_id;
    bignet.rocid = this_roc_id;
    printf("codaDownload: set this_roc_id = %d, rocId() can be called from now on\n",this_roc_id);

    /* zero output pointer; will be used to link ROLs */
	/**
    last_output = (ROL_MEM_PART **) NULL;
	**/


    /* loop over readout lists */
    for(ix=0, jx=0; ix<nrols; ix++, jx++)
    {
      rolP = rocp->rolPs[jx] = rolPs[jx];
      if(rolP == NULL)
      {
        printf("download: malloc error - return\n");
        TRANSITION_UNLOCK;
        return(CODA_ERROR);
      }

      /* cleanup rolP structure */
      memset((char *) rolP, 0, sizeof(ROLPARAMS));

      /* Split list into rol object file and user string */
      if(listSplit1(listArgv[ix], 0, &tmpArgc, tmpArgv))
      {
        TRANSITION_UNLOCK;
        return(CODA_ERROR);
	  }

      if(tmpArgc != 2)
      {
        printf("ERROR: Incorrect number of Arguments passed for ROL = %d\n",
                 tmpArgc);
        TRANSITION_UNLOCK;
        return(CODA_ERROR);
      }

      if(ix==0)
      {
        /* set parent component name in readout list */
        rolP->name = object->name;
        sprintf(name,"rol%d",jx);
        strcpy(rolP->tclName,name);
        strncpy(rolP->confFile,confFile,255);

        /* initialize rol parameters structure, memory partitions */
        rolP->pid = object->codaid;

        /*set classid based on object class so rols will know if they 'master' or 'slave'*/
        if(!strcmp(object->className,"ROC"))     rolP->classid = 0; /* slave */
        else if(!strcmp(object->className,"TS")) rolP->classid = 1; /* master */
        else                                     rolP->classid = 2; /* standalone */

        /* setup pointers to global ROC information */
        rolP->nevents = (uint32_t *) &(object->nevents);
		/* NOT IN USE !!!???
        rolP->async_roc = &(rocp->async_roc_flag);
		*/

        /* load ROL1 */
        res = codaLoadROL(rolP, tmpArgv[0], tmpArgv[1]);
        if(res)
	    {
          TRANSITION_UNLOCK;
          return(CODA_ERROR);
	    }

		printf("coda_roc 1\n"); fflush(stdout);

        /* execute ROL init procedure (described in 'rol.h') */
        rolP->daproc = DA_INIT_PROC;
		printf("coda_roc 11\n"); fflush(stdout);
		printf("coda_roc 12: 0x%08x\n",rolP); fflush(stdout);
		printf("coda_roc 13: 0x%016x 0x%016x\n",rolP->rol_code,*(rolP->rol_code)); fflush(stdout);
        (*(rolP->rol_code)) (rolP);

		printf("coda_roc 2\n"); fflush(stdout);

        /* check if initialization was successful */
        if(rolP->inited != 1)
        {
          tcpState = rocp->state = DA_CONFIGURED;
          printf ("ERROR: ROL initialization failed\n");
          TRANSITION_UNLOCK;
          return(CODA_ERROR);
        }

		printf("coda_roc 3\n"); fflush(stdout);

        /* execute ROL1 download procedure */
        rolP->daproc = DA_DOWNLOAD_PROC;
        (*(rolP->rol_code)) (rolP);

		printf("coda_roc 4\n"); fflush(stdout);

	  }

      if(ix==1)
      {
        /* copy rol2's name into BIGNET structures to be executed from 'proc' */
        strncpy(bigproc.rolname,tmpArgv[0],255);
        strncpy(bigproc.rolparams,tmpArgv[1],127);
        strncpy(bignet.rolname,tmpArgv[0],255);
        strncpy(bignet.rolparams,tmpArgv[1],127);
      }

    }

    printf("codaDownload: downloaded\n");
  }
  else
  {
    printf("WARN: no readout lists in current configuration\n");
  }


/* connect to database */
printf("mysql_host >%s<\n",mysql_host);fflush(stdout);
dbsock = dbConnect(mysql_host, expid);
printf("3123-1: dbsock=%d\n",dbsock);fflush(stdout);
if(dbsock==NULL)
{
  printf("cannot connect to the database 3 - exit\n");
  exit(0);
}


  /* Sergey: define 'rocp->async_roc_flag'  using DB where that information
  exists; that setting was removed from ROL1's Download() procedure */
  sprintf(tmpp,"SELECT outputs FROM %s WHERE name='%s'",
    confname,object->name);

  printf("query >%s<\n",tmpp);
  if(dbGetStr(dbsock, tmpp, tmp)==CODA_ERROR)
  {
    printf("error !!!\n");
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }


  if(strlen(tmp)==0)
  {
    printf("outputs tmp=0\n");
    rocp->output_switch = 3;
    rocp->async_roc_flag = 1;
  }
  else if(strncmp(tmp,"none",4)==0)
  {
    printf("outputs 1 >%s<\n",tmp);
    rocp->output_switch = 3;
    rocp->async_roc_flag = 1;
  }
  else
  {
    printf("outputs 2 >%s<\n",tmp);
    rocp->output_switch = 0;
    rocp->async_roc_flag = 0;
  }

  printf("++++++++++++++++++++ outputs >%s< -> async_flag=%d\n",
    tmp,rocp->async_roc_flag);



  /*sergey temporary ???????????????????????????
rocp->output_switch = 0;
  */


/* let NIOS to be async 
#ifndef Linux_nios2
rocp->async_roc_flag = 0;
#endif
*/


  /**************************************************************/
  /* update 'inuse' column in 'configname' table in database:   */
  /* for the sync ROC, set 'inuse'=rocid, otherwise set it 'no' */
  /**************************************************************/
  if(rocp->async_roc_flag == 0)
  {
    sprintf(tmpp,"UPDATE %s SET inuse='%d' WHERE name='%s'",
      configname,object->codaid,object->name);

    printf("DB command >%s<\n",tmpp);
  }
  else
  {
    sprintf(tmpp,"UPDATE %s SET inuse='no' WHERE name='%s'",
      configname,object->name);
    printf("DB command >%s<\n",tmpp);
  }
  if(mysql_query(dbsock, tmpp) != 0)
  {
    printf("ERROR: cannot UPDATE 'configname' SET inuse='yes'\n");
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }

  /*****/
  /*   */
  /*****/
  tcpState = rocp->state = DA_DOWNLOADED;
  rocp->active = 0;
/**
  partReInitAll();
**/
  /*rocp->last_event = 0;*/ /* will do it in Prestart again ??? */
  last_event_check = 0;

  /******************** VERY IMPORTANT:
  here we will set how 'coda_proc' and 'coda_net' will communicate to surrounding
  and/or to each other; 'dabufp' is set here !!!!!!!!!!!!*/


  /* 'bigproc' will be used as 'proc_thread' structure, 'dabufp' will be pointing
  to it's buffer, gbigBUF will be used to pass data from coda_proc to coda_net */
  if(gbigDMA != NULL)
  {
    bb_init(&gbigDMA);
    dabufp = bb_write_current(&gbigDMA);
    if(dabufp == NULL)
    {
      printf("ERROR in bb_write_current: FAILED\n");
      TRANSITION_UNLOCK;
      return(CODA_ERROR);
    }
  }

  if(gbigBUF != NULL)
  {
    bb_init(&gbigBUF);

    /* output from coda_proc goes to gbigBUF */
    bigproc.gbigout = gbigBUF;
  }

  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");fflush(stdout);



  dabufp += BBHEAD;
  printf("1: dabufp set to 0x%x\n",dabufp);

  rocp->active = 1;

printf("999: %d %d %d %d\n",rocp->state,DA_ENDING,clear_to_send,rocp_primefd);fflush(stdout);

  /*sergey*/
  bigproc.failure = 0;
  bignet.failure = 0;

  if(rocp->async_roc_flag == 0)
  {
    /* Spawn the rols_loop Thread */
    {
      pthread_attr_init(&detached_attr);
      pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
      pthread_attr_setscope(&detached_attr,PTHREAD_SCOPE_SYSTEM/*PTHREAD_SCOPE_PROCESS*/);
      pthread_create( /*(unsigned int *)*/ &iTaskROL, &detached_attr,
		   (void *(*)(void *)) rols_loop, (void *) NULL);
    }
  }

  /* use codaUpdateStatus here ??????? */
  /* update 'state' column in 'process' table in database */
  sprintf(tmpp,"UPDATE process SET state='downloaded' WHERE name='%s'",
    object->name);
printf("DB command >%s<\n",tmpp);
  if(mysql_query(dbsock, tmpp) != 0)
  {
    printf("ERROR: cannot UPDATE process SET state='downloaded'\n");
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }
  else
  {
    printf("UPDATE process success\n");
  }

  /* disconnect from database */
  dbDisconnect(dbsock);


  printf("ROL1 POLL: %d\n",rocp->rolPs[0]->poll);
  /* if ROL2 was specified, download proc */
  if(nrols==2)
  {
    printf("ROL2 POLL: %d\n",rocp->rolPs[1]->poll);

    bigproc.gbigin = gbigDMA;
    proc_download(bigproc.rolname,bigproc.rolparams,rocId());
  }
  else
  {
    bignet.gbigin = gbigDMA;
  }


  if(codaUpdateStatus("downloaded") != CODA_OK)
  {
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }

  TRANSITION_UNLOCK;

  return(CODA_OK);
}


/******************************************************************************
 *
 * This routine informs the EB of state changes in a particular ROC
 */

void
informEB(objClass object, unsigned int mTy, unsigned int mA, unsigned int mB)
{
  ROLPARAMS  *rolP;

  uint32_t *data, len;
  rocParam rocp = (rocParam) object->privated;
  int res, ii;
  unsigned int id;
  uint32_t *bigbuf;
  char *chbuf;
  int len_in_words;

  printf("informEB reached\n");


  /*ERRORRRRRRRRRRRRRRRRRRRRRRR*/
  bigbuf = bb_write_current(&gbigDMA);
  if(bigbuf == NULL)
  {
    printf("informEB: alloc error - return\n");
    return;
  }

  len = 5; /* full bank length in words */


#undef INSERT_CONFIG_FILE

  /* if TS/Standalone and Prestart, insert run confFile */
#ifdef INSERT_CONFIG_FILE

  if(mTy==(unsigned int)EV_PRESTART)
  {
    printf("confFile >%s<\n",confFile);
    if( strncmp(confFile,"none",4) && strncmp(confFile,"NONE",4) )
    {
      if((rolP = rocp->rolPs[0]) != NULL)
      {
        if( rolP->classid > 0 )
        {
	      chbuf = loadwholefile(confFile, &len_in_words);
          if(chbuf == NULL)
	      {
            printf("ERROR: coda_roc: cannot read conffile - does not insert it into data stream !!!\n");
	      }
          else
	      {
            strncpy((char *)&bigbuf[BBHEAD+5],chbuf,(len_in_words<<2));
            free(chbuf);

            len += len_in_words;
	      }
        }
      }
    }
  }

#endif

  bigbuf[BBIWORDS] = (BBHEAD_BYTES+(len*4))>>2;
  bigbuf[BBIBUFNUM] = -1;
  bigbuf[BBIROCID]  = object->codaid;
  bigbuf[BBIEVENTS] = 1;  /* # events in buffer = 1 */
  bigbuf[BBIFD]     = rocp_primefd;
  if(mTy == (unsigned int) EV_END)
  {
    bigbuf[BBIEND]  = 1;
  }
  else
  {
    bigbuf[BBIEND]  = 0;
  }
  bigbuf[BBHEAD]    = (len-1);  /* event starts here; contains # words in event */
  bigbuf[BBHEAD+1]  = CTL_BANK_HDR(mTy);  /* control bank header */
  bigbuf[BBHEAD+2]  = 1200; /* some junk */
  bigbuf[BBHEAD+3]  = mA;   /* parameter a */
  bigbuf[BBHEAD+4]  = mB;   /* parameter b */

printf("informEB: %d %d %d %d %d %d - 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
bigbuf[0],bigbuf[1],bigbuf[2],bigbuf[3],bigbuf[4],bigbuf[5],
bigbuf[6],bigbuf[7],bigbuf[8],bigbuf[9],bigbuf[10],bigbuf[11]);
  
  bigbuf = bb_write(&gbigDMA);

  return;
}


/******************************************************************************
 * This routine is called by RC as the prestart transition
 */

int
codaPrestart()
{
  objClass object = localobject;

  rocParam    rocp;
  ROLPARAMS  *rolP;
  int         ix, state, iii, port,ret;
  MYSQL *dbsock;
  char        tmp[100];
  char        tmpp[1000];
  char host[128];

  TRANSITION_LOCK;

  printf("roc_prestart reached\n");

  rocp = (rocParam) object->privated;
  bigproc.doclose = 0;
  bignet.doclose = 0;



/*sergey*/
  if(nrols>1)
  {
    bb_init(&bigproc.gbigin);
    bigproc.failure = 0;
  }
  else
  {
    bb_init(&bignet.gbigin);
  }
  bignet.failure = 0;
  last_event_check = 0;




/**
  partReInitAll();
**/

  tcpState = rocp->state = DA_PRESTARTING;

  rocCleanup();



  /***********************/
  /* connect to database */
  /***********************/

  dbsock = dbConnect(mysql_host, expid);
  printf("315: dbsock=%d\n",dbsock);
  if(dbsock==NULL)
  {
    printf("cannot connect to the database 5 - exit\n");
    exit(0);
  }


  /**********************/
  /* Get the run number */
  /**********************/
  sprintf(tmpp,"SELECT runNumber FROM sessions WHERE name='%s'",session);
  if(dbGetInt(dbsock, tmpp, &(object->runNumber))==CODA_ERROR)
  {
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }
  printf(">>> prestarting, run %d, type %d\n",object->runNumber,object->runType);


  /* set token Interval (it was obtained from DB at 'Download') */
  bigproc.token_interval = token_interval;
  bignet.token_interval = token_interval;


  /* get rocMask from database's options table; it must be set by EB;
  will be used in TS ROC only, other ROCs do not need that information */
  sprintf(tmpp,"SELECT value FROM %s_option WHERE name='rocMask'",configname);
  if(dbGetInt(dbsock, tmpp, &rocMask)==CODA_ERROR)
  {
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }
  printf("rocMask=0x%08x\n",rocMask);


  /* open TCP link to the Event Builder */
  sprintf(tmpp,"SELECT outputs FROM %s WHERE name='%s'",configname,object->name);

  printf("!!!!!!!!!!!!! tmpp >%s<\n",tmpp);
  printf("!!!!!!!!!!!!! tmpp >%s<\n",tmpp);
  printf("!!!!!!!!!!!!! tmpp >%s<\n",tmpp);

  if(dbGetStr(dbsock, tmpp, tmp)==CODA_ERROR)
  {
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }

  /* disconnect from database */
  dbDisconnect(dbsock);



  if(rocp->async_roc_flag == 0)
  {
    printf("++++> output to network\n");

    for(ix=0; ix<strlen(tmp); ix++)
    {
      if(tmp[ix]==':')
      {
        tmp[ix] = '\0';
        break;
      }
    }

    rocCloseLink(); /* empty anyway */

    printf("!!!!!!!!!!!!! for rocOpenLink: >%s< >%s< %d\n",object->name,tmp,
      strlen(tmp));
    printf("!!!!!!!!!!!!! for rocOpenLink: >%s< >%s<\n",object->name,tmp);
    printf("!!!!!!!!!!!!! for rocOpenLink: >%s< >%s<\n",object->name,tmp);
    printf("!!!!!!!!!!!!! for rocOpenLink: >%s< >%s<\n",object->name,tmp);

    /* open TCP to EB here */
	{
      ret = rocOpenLink(object->name, tmp, host, &port, &socketnum);
      if(ret<0)
	  {
        /*REDO IT USING UDP_user_request !!!*/
        char tmpp[1000];
        strcpy(tmpp,"err:");
        strcat(tmpp,object->name);
        strcat(tmpp," ");
        strcat(tmpp,"CANNOT ESTABLISH TCP TO EB");
        UDP_request(tmpp);
        sleep(2);
        UDP_cancel(tmpp);

        printf("roc_component ERROR: CANNOT ESTABLISH TCP TO EB !!!\n");
        return(CODA_ERROR);
	  }
      if(socketnum>0)
	  {
        /* why needs that ???*/
        bigproc.socket = socketnum;
        bignet.socket = socketnum;
        bigproc.port = port;
        strcpy((char *)bigproc.host, host);
        bignet.port = port;
        strcpy((char *)bignet.host, host);
	  }
	}
  }
  else
  {
    printf("coda_roc: ++++> output to nowhere\n");
  }



  printf("Spawn proc/net threads\n");
  printf("Spawn proc/net threads\n");
  printf("Spawn proc/net threads\n");


  /* spawn 'proc' or/and 'net' threads */
  /* IMPORTANT: 'net' must be started first !!! (realy ???) */

  if(nrols>1)
  {
	printf("calling proc_prestart\n");fflush(stdout);
    proc_prestart(rocId());
    printf("proc_prestart done\n");fflush(stdout);
  }

  /* TODO: delete 'net_thread' if running */
  sleep(1);



  if(rocp->async_roc_flag == 0)
  {
    if(nrols>1) /* have rol2 -> input to coda_net goes from gbigBUF */
	{
      bignet.gbigin = gbigBUF;
	}
    else
	{
      bignet.gbigin = gbigDMA; /* no rol2 -> input to coda_net goes from gbigDMA */
	}
    bignet.gbigout = NULL; /* no output from coda_net */
    bb_init(&bignet.gbigin);
    bignet.failure = 0;

    pthread_attr_init(&detached_attr);
    pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(&detached_attr,PTHREAD_SCOPE_SYSTEM/*PTHREAD_SCOPE_PROCESS*/);
    iii=pthread_create( /*(unsigned int *)*/ &iTaskNET, &detached_attr,
		   (void *(*)(void *)) net_thread, (void *) &bignet);
    printf("codaPrestart: net thread returned %d\n",iii);

    /* TODO: delete 'proc_thread' if running */
    sleep(1);

    /* have rol2 -> start proc thread */
	if(nrols>1)
	{
      bb_init(&bigproc.gbigin);
      bigproc.failure = 0;

      pthread_attr_init(&detached_attr);
      pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
      pthread_attr_setscope(&detached_attr,PTHREAD_SCOPE_SYSTEM/*PTHREAD_SCOPE_PROCESS*/);
      iii=pthread_create( /*(unsigned int *)*/ &iTaskPROC, &detached_attr,
		   (void *(*)(void *)) proc_thread, (void *) &bigproc);
      printf("codaPrestart: proc thread returned %d\n",iii);
	}
  }


  object->nevents = 0; /* zero bookeeping counters */
  object->nlongs = object_nlongs = 0;
  rocp_recNb = 0;
  /*rocp->last_event = 0;*/
  last_event_check = 0;

  /* ROL1: set bookeeping parameters then call prestart routine */
  if((rolP = rocp->rolPs[0]) != NULL)
  {
    rolP->runNumber = object->runNumber;
    rolP->runType = object->runType;
    rolP->recNb = 0;

    rolP->daproc = DA_PRESTART_PROC;
    (*rolP->rol_code) (rolP);
  }


  /* save socket number */
  if(rocp->output_switch == 0)
  {
    printf("111\n");
    rocp_primefd = socketnum;
    printf("333: rocp_primefd=%d\n",rocp_primefd);
  }
  else
  {
    rocp_primefd = 0;
  }

  /* If the async_roc flag is set we don't send control events */
  tcpState = rocp->state = DA_PRESTARTED;
  if(rocp->async_roc_flag == 0)
  {
    informEB(object, (unsigned int) EV_PRESTART, (unsigned int) object->runNumber,
             (unsigned int) object->runType);
  }

  if(codaUpdateStatus("paused") != CODA_OK)
  {
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }
  printf("prestarted\n");fflush(stdout);

  TRANSITION_UNLOCK;

  return(CODA_OK);
}



/* when end clicked, following printed (GOES FIRST, BEFORE EB):

CODAtcpServer: start work thread
CODAtcpServer: befor: socket=5 address>129.57.86.64< port=58891
CODAtcpServer: wait: coda request >go< in progress
CODAtcpServerWorkTask entry
codaExecute reached, message >end<, len=3
codaExecute: 'end' transition
codaEnd reached ??
codaEnd reached !!
codaEnd: calling ROL1's end (0x617c969a)
INIT_NAME: INFO: rolp->daproc = 3
testtdisable reached

  Read 496498 events

INFO: User End 1 Executed
codaEnd: finished ROL1's end
codaUpdateStatus: dbConnecting ..
codaUpdateStatus: dbConnect done

codaUpdateStatus(table 'process'): >UPDATE process SET state='ending' WHERE name='ROC85'<
codaUpdateStatus: dbDisconnecting ..
codaUpdateStatus: dbDisconnect done
codaUpdateStatus: updating request ..
UDP_standard_request >sta:ROC85 ending<
UDP_standard_request >sta:ROC85 ending<
UDP_standard_request >sta:ROC85 ending<
UDP_standard_request >sta:ROC85 ending<
UDP_standard_request >sta:ROC85 ending<
UDP_standard_request >sta:ROC85 ending<
UDP_cancel: cancel >sta:ROC85 active<
codaUpdateStatus: updating request done
codaEnd: NOW !!!!!!!!!!!!!!!!!!!
codaEnd: NOW !!!!!!!!!!!!!!!!!!!
codaEnd: NOW !!!!!!!!!!!!!!!!!!!
codaEnd: NOW !!!!!!!!!!!!!!!!!!!
codaEnd: NOW !!!!!!!!!!!!!!!!!!!
codaEnd: unlocking
codaEnd: done
codaExecute done
CODAtcpServerWorkTask exit ?
CODAtcpServerWorkTask exit !

  3: ROLS_LOOP LOCKed, buffer sent
  3: ROLS_LOOP UNLOCKed

coda_roc: DA_ENDING before lock
coda_roc: DA_ENDING after lock
coda_roc: in DA_ENDING last event=496498 nevents=496498
coda_roc: in DA_ENDING rocp->active=2
coda_roc: call informEB(EV_END)
informEB reached
informEB: 11 -1 85 1 13 1 - 0x00000004 0x009401cc 0x000004b0 0x00000203 0x00079372 0x00000001
Inserted End event on queue
codaUpdateStatus: dbConnecting ..
net_thread: END condition received
NET_THREAD GOT END 0 SEC
codaUpdateStatus: dbConnect done
codaUpdateStatus(table 'process'): >UPDATE process SET state='downloaded' WHERE name='ROC85'<
codaUpdateStatus: dbDisconnecting ..
codaUpdateStatus: dbDisconnect done
codaUpdateStatus: updating request ..
UDP_standard_request >sta:ROC85 downloaded<
UDP_standard_request >sta:ROC85 downloaded<
UDP_standard_request >sta:ROC85 downloaded<
UDP_standard_request >sta:ROC85 downloaded<
UDP_standard_request >sta:ROC85 downloaded<
UDP_standard_request >sta:ROC85 downloaded<
UDP_cancel: cancel >sta:ROC85 ending<
codaUpdateStatus: updating request done
ended after 496498 events
roc_network cleanup +++++++++++++++++++++++++++++++++++++ 1
bb_cleanup 0: 0x00622af0
bb_cleanup 1: 0x00bc8510
bb_cleanup 2
roc_network +++++++++++++++++++++++++++++++++++++ 2
LINK_close: socket #13 connection closed
roc_network +++++++++++++++++++++++++++++++++++++ 3
WRITE THREAD EXIT
call: 'ROC85 close_links'
rocCloselink reached

*/



/******************************************************************************
 * This routine is called by RC
 * to implement end transition
 */

int
codaEnd()
{
  objClass object = localobject;

  rocParam    rocp;
  ROLPARAMS  *rolP;
  int         ix, state;
  MYSQL *dbsock;
  char      tmpp[1000];

  printf("codaEnd reached ??\n");fflush(stdout);

  TRANSITION_LOCK;

  rocp = (rocParam) object->privated;

  printf("codaEnd reached !!\n");fflush(stdout);

  /* end ROL1 */
  if((rolP=rocp->rolPs[0]) != NULL)
  {
    if(rolP->inited)
    {
      rolP->daproc = DA_END_PROC;
      printf("codaEnd: calling ROL1's end (0x%08x)\n",(*rolP->rol_code));fflush(stdout);
      (*rolP->rol_code) (rolP);
      printf("codaEnd: finished ROL1's end\n");fflush(stdout);
    }
  }


  /*sergey 2016 
  if(nrols>1)
  {
    printf("calls 'proc_end', rocid=%d\n",rocId());
    proc_end(rocId());
  }
  sergey 2016 */



  if((rocp->async_roc_flag == 1))
  {
    printf("codaEnd: go to Downloaded for async roc\n");fflush(stdout);
    if(codaUpdateStatus("downloaded") != CODA_OK)
    {
      TRANSITION_UNLOCK;
      return(CODA_ERROR);
	}
    tcpState = rocp->state = DA_DOWNLOADED;
    rocp->active = 1;
  }
  else
  {

    /******************************/
    /* delay inside following ??? */
    /******************************/
    if(codaUpdateStatus("ending") != CODA_OK)
    {
printf("ERROR_ENDING ...\n");
printf("ERROR_ENDING ...\n");
printf("ERROR_ENDING ...\n");
printf("ERROR_ENDING ...\n");
printf("ERROR_ENDING ...\n");fflush(stdout);

      TRANSITION_UNLOCK;
      return(CODA_ERROR);
	}

    printf("codaEnd: NOW !!!!!!!!!!!!!!!!!!!\n");
    printf("codaEnd: NOW !!!!!!!!!!!!!!!!!!!\n");
    printf("codaEnd: NOW !!!!!!!!!!!!!!!!!!!\n");
    printf("codaEnd: NOW !!!!!!!!!!!!!!!!!!!\n");
    printf("codaEnd: NOW !!!!!!!!!!!!!!!!!!!\n");fflush(stdout);
    tcpState = rocp->state = DA_ENDING;

    rocp->active = 2; /* ??? it must go AFTER setting 'ending', otherwise it may
                      set 'downloaded' really fast and then we will rewrite
                      it by 'ending' so 'ending' will be last status to be
                      reported to EB (must be 'downloaded') */
  }


  /*DELETE_PROC*/;

  printf("codaEnd: unlocking\n");fflush(stdout);
  TRANSITION_UNLOCK;
  printf("codaEnd: done\n");fflush(stdout);

  /*
  printf("codaEnd: give a time to readout the rest ...\n");fflush(stdout);
  sleep(2);
  printf("codaEnd: now exits\n");fflush(stdout);
  */

  return(CODA_OK);
}







/******************************************************************************
 * This routine is called by RC
 * to implement the pause transition
 */

int
codaPause()
{
  objClass object = localobject;

  rocParam        rocp;
  ROLPARAMS      *rolP;
  int             ix;

  TRANSITION_LOCK;

  rocp = (rocParam) object->privated;

  /* pause ROL1 */
  if((rolP = rocp->rolPs[0]) != NULL)
  {
    rolP->daproc = DA_PAUSE_PROC;
    (*rolP->rol_code) (rolP);
  }

  rocp->active = 2;

  if((rocp->async_roc_flag == 1))
  {
    tcpState = rocp->state = DA_PAUSED;
    if(codaUpdateStatus("paused") != CODA_OK)
    {
      TRANSITION_UNLOCK;
      return(CODA_ERROR);
	}
  }
  else
  {
    tcpState = rocp->state = DA_PAUSING;
    if(codaUpdateStatus("pausing") != CODA_OK)
    {
      TRANSITION_UNLOCK;
      return(CODA_ERROR);
	}
  }

  TRANSITION_UNLOCK;

  return(CODA_OK);
}

/******************************************************************************
 * This routine is called by RC
 * to implement the go transition
 */

int
codaGo()
{
  objClass object = localobject;

  rocParam    rocp;
  ROLPARAMS  *rolP;
  int         ix, state, ticks;

  TRANSITION_LOCK;

  rocp = (rocParam) object->privated;

  printf("activating ..\n");

  if(rocp->async_roc_flag == 0)
  {
    informEB(object, (unsigned int) EV_GO, (unsigned int) 0, (unsigned int) object->nevents);
  }



  g_events_in_buffer = 0;
  /* set pointer to the first buffer (must be set BEFORE ROLs are executed) */

  /*ERRORRRRRRRRR*/
  dabufp = bb_write_current(&gbigDMA);
  if(dabufp == NULL)
  {
    printf("ERROR in bb_write_current: FAILED\n");
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }
  dabufp += BBHEAD;
  printf("1-1: dabufp set to 0x%x\n",dabufp);


  /* Go ROL1 - old location */

  if(nrols>1)
  {
    printf("calls 'proc_go', rocid=%d\n",rocId());
    proc_go(rocId());
  }

  TRANSITION_UNLOCK;


  /***********/
  /* Go ROL1 */

  /* in polling mode, go() will be executed before starting rols_loop,
	 in interrupt mode - in opposite order */

  if((rolP = rocp->rolPs[0]) != NULL)
  {
    if(rolP->poll)
    {
      rolP->daproc = DA_GO_PROC;
      (*rolP->rol_code) (rolP);
    }    
  }


  rocp->active = 2; /* will start ROLS_LOOP */
  tcpState = rocp->state = DA_ACTIVE;
  if(codaUpdateStatus("active") != CODA_OK)
  {
    TRANSITION_UNLOCK;
    return(CODA_ERROR);
  }
  printf("active, events so far %d\n",object->nevents);fflush(stdout);


  if((rolP = rocp->rolPs[0]) != NULL)
  {
    if(!rolP->poll)
    {
      /*without following delay we are hunging on the first event in standalone mode with interrupts;
      probably have to give a time to rols_loop to start after setting 'rocp->active=2'
      and before first interrupt; need to synchronize them better ... */
      sleep(1);
      rolP->daproc = DA_GO_PROC;
      (*rolP->rol_code) (rolP);
    }
  }

  return(CODA_OK);
}



/*************/
/* MAIN LOOP */
/*************/

#define LOCKINTS
#define UNLOCKINTS



#define SET_TIMEOUT(cba) \
    if(object->nevents < 10) \
    { \
      timeout = gethrtime() + 1*100000; \
	  /*printf("SET_TIMEOUT 3: timeout=%lld\n",timeout);*/	\
    } \
    else \
    { \
      timeout = gethrtime() + (token_interval/60)*100000;	/*1000000 - 1sec*/	\
	  /*printf("SET_TIMEOUT 4: timeout=%lld\n",timeout);*/				\
    }

/*
#define SET_TIMEOUT \
    if(object->nevents < 10)			\
    { \
      timeout = time(0) + 1;	\
    } \
    else \
    { \
      timeout = time(0) + token_interval/60;	\
    }
*/


/* NOTES:

 1. loop over readout lists will be performed if(rocp->active > 1); 
   'Go' transition sets rocp->active=2

 2. 

*/

void
tloopprint()
{
  printf("tdisp = %u %u %u %u %u %u\n",tloop1,tloop2,tloop3,tloop4,tloop5,tloop6);
}

void
rols_loop()
{
  objClass object = localobject;
#ifdef Linux
  prctl(PR_SET_NAME,"coda_rols");
#endif
  printf("rols_loop started\n");

  /* some initialization */
  icycle3=0;
  dataInBuf = BBHEAD_BYTES;
  g_events_in_buffer = 0;
  clear_to_send = 0;
  SET_TIMEOUT(88);

  tloop1=0;
  tloop2=0;
  tloop3=0;
  tloop4=0;
  tloop5=0;
  tloop6=0;
  while(1)
  {
    setHeartBeat(HB_ROL,0,5);
    if(rols_loop_exit)
    {
      setHeartBeat(HB_ROL,0,-1);
      rols_loop_exit = 0;
      return;
    }
    setHeartBeat(HB_ROL,1,5);
    tloop1++;
    output_proc_network(0);

  }

  setHeartBeat(HB_ROL,0,-1);

  return;
}

#define SEND_BUFFER_(abc) \
  /*SENDING big buffer*/				 \
  if(clear_to_send && (rocp_primefd>=0)) \
  { \
    /*setHeartBeat(HB_ROL,14,5);*/ \
    /* send current output buffer */ \
    /* set now 'dabufp' to the beginning of */ \
    /* 'big' buffer just to fill a header   */ \
	/*prints all the time*/ \
    /*printf("1 SEND_BUFFER_ROC %d (0x%08x)\n",abc,dabufp);*/	\
    dabufp = bb_write_current(&gbigDMA); \
    /*printf("2 SEND_BUFFER_ROC %d (0x%08x %d) (conditions %d %d)\n",abc,dabufp,dabufp[0],clear_to_send,rocp_primefd);*/ \
    /*setHeartBeat(HB_ROL,15,5);*/ \
    if(dabufp == NULL) \
    { \
      printf("ERROR in bb_write: FAILED1\n"); \
      /*setHeartBeat(HB_ROL,0,-1);*/ \
      SENDBUFFER_UNLOCK; \
      return; \
    } \
    /* fill 'big' buffer header */ \
    /*printf("FILLING HEADER: dataInBuf>>2=%d, g_events_in_buffer=%d\n",dataInBuf>>2,g_events_in_buffer);*/ \
    dabufp[BBIWORDS] = dataInBuf>>2;        /* buffer length in words */ \
    /*printf("SSS1 %d (0x%08x %d)\n",abc,dabufp,dabufp[0]);*/ \
    dabufp[BBIBUFNUM] = ++ rocp_recNb;      /* buffer number */ \
    dabufp[BBIROCID]  = this_roc_id/*object->codaid*/;     /* ROC id */ \
    dabufp[BBIEVENTS] = g_events_in_buffer; /* the number of events */ \
	if(dabufp[BBIEVENTS]<=0) {printf("ERROR(coda_roc.c): nev=%d\n",dabufp[BBIEVENTS]);exit(0);} \
    dabufp[BBIFD]     = rocp_primefd; \
    dabufp[BBIEND]    = 0; \
    /*printf("ROC: %d %d %d %d 0x%08x\n",dabufp[0],dabufp[1],dabufp[2],dabufp[3],dabufp[4]);*/ \
	/*bb_check(dabufp);*/  \
	/* main send */ \
    /*printf("SSS2 %d (0x%08x %d)\n",abc,dabufp,dabufp[0]);*/ \
    if(dabufp[BBIWORDS] > BBHEAD) \
    { \
      /* prints all the time */ \
	  /*printf("coda_roc: calling 'bb_write', MAY WAIT INSIDE FOREVER IF THERE IS NO SPACE !!!!!!!!!!!!!!!!!!!\n");*/ \
      dabufp = bb_write(&gbigDMA); \
      if(dabufp == NULL) \
      { \
        printf("INFO from bb_write: RETURN 0\n"); \
        /*setHeartBeat(HB_ROL,0,-1);*/ \
        SENDBUFFER_UNLOCK; \
        return; \
      } \
    } \
    else \
    { \
      printf("attempt to send short buffer failed !!!, dabufp[BBIWORDS]=%d\n",dabufp[BBIWORDS]); \
    } \
    /* cleanup 'event counter in buffer' */ \
    g_events_in_buffer = 0; \
    /* determine total # of bytes to be sent */    \
    object_nlongs += dataInBuf >> 2; \
    /* pickup next buffer */ \
    dabufp += BBHEAD; \
    /* reserve space for header */ \
    dataInBuf = BBHEAD_BYTES; \
    clear_to_send = 0; \
    SET_TIMEOUT(abc); \
  } \
  /*setHeartBeat(HB_ROL,0,-1)*/


void 
output_proc_network(int dummy)
{
  objClass object = localobject;
  rocParam rocp;
  ROLPARAMS *rolP;
  int i, lockKey, status, len, ii;
  int itmp;
  char tmpp[1000];

/* timing */
#ifndef Darwin
  static hrtime_t start, end, time1, time2, time03, nevent;
  static int nev;
  static hrtime_t sum;
#endif
  /*
  printf("00: rocp->active=%d\n",rocp->active);fflush(stdout);
  rocStatus();fflush(stdout);
  */
  /*
printf("output_proc_network reached\n");fflush(stdout);
  */
  cycle = 10;
  cycle3 = 100000;

  /* Restore pointer to roc private storage */
  rocp = (rocParam) object->privated;
  /*
printf("000: %d %d %d %d\n",rocp->state,DA_ENDING,clear_to_send,rocp_primefd);
  */
  /* if thread(s) returns error: */
  if(bigproc.failure || bignet.failure)
  {
    printf("ERROR: bigproc.failure=%d, bignet.failure=%d\n",bigproc.failure,bignet.failure);fflush(stdout);
    fflush(stdout);
    rocp->active = 0;
    bigproc.failure = 0;
    bignet.failure = 0;

	/* do not call transitions from here - it is deadlock !!!
    set some flag or something - TODO !!!!!!!!!!!!!!!!!
    codaEnd();
	*/

printf("\nCalls bb_cleanup.\n");
printf("Calls bb_cleanup.\n");
printf("Calls bb_cleanup.\n\n");

if(nrols>1) bb_cleanup(&bigproc.gbigin);
else bb_cleanup(&bignet.gbigin);


    setHeartBeat(HB_ROL,0,-1);
    return;
  }


  /* Check if we have any events on output queue */
  if(rocp->active > 1)
  {
    setHeartBeat(HB_ROL,2,5);

    /* We should always have a primary list but allow for not just
    in case. We also should stop polling if ending. */
    rolP = rocp->rolPs[0]; /* rol1 */
    if(rolP==0)
    {
      printf("coda_roc: ERROR FATAL: no ROL1: rol1=0x%08x\n",rolP);fflush(stdout);
      fflush(stdout);
      return;
    }
 

    /************************************************************************/
    /* Readout list Polling Loop -- Stay in this loop until we need to send */


    /**********************************************/
    /**********************************************/
    /**********************************************/
    nevent = time1 = time2 = 0;
    do
    {
      tloop2++;
      start = gethrtime();
      setHeartBeat(HB_ROL,3,5);

	  /*
	  printf("11: rocp->active=%d\n",rocp->active);fflush(stdout);
      rocStatus();fflush(stdout);
	  */

	  /*****************/
      /* start of ROL1 */
      setHeartBeat(HB_ROL,4,5);
      if(rocp->state != DA_ENDING)
      {
        /* Do we actually need to poll ?? 
        Don't poll if this is an interrupt driven list
        Don't poll if we have no free buffers ('pool->list.c' is # of event bufs) */
        if(rolP->poll/* && rolP->pool->list.c*/)
        {
          setHeartBeat(HB_ROL,21,5);
#ifdef Linux_vme
#ifdef NEW_ROC
bla2
          dabufp_usermembase = bb_get_usermembase(&gbigDMA);
          dabufp_physmembase = bb_get_physmembase(&gbigDMA);
#endif
#endif

      tloop3++;
TRANSITION_LOCK;
          rolP->daproc = DA_POLL_PROC;
	      /*printf("11: befor ROL1\n");fflush(stdout);*/
      tloop4++;
          (*rolP->rol_code) (rolP); /* pseudo-trigger cdopoll */
      tloop5++;
	      /*printf("11: after ROL1\n");fflush(stdout);*/
TRANSITION_UNLOCK;
      tloop6++;

          setHeartBeat(HB_ROL,22,5);
        }


		/*
..........................................
[1] bb_write_current (in):  write=4 read=8
[1] bb_write_current (out): write=4 read=8
[1] bb_write (in):          write=4 read=8
[1] bb_write (out):         write=5 read=8

[1] bb_write_current (in):  write=5 read=8
[1] bb_write_current (out): write=5 read=8
[1] bb_write (in):          write=5 read=8
[1] bb_write (out):         write=6 read=8

[1] bb_write_current (in):  write=6 read=8
[1] bb_write_current (out): write=6 read=8
[1] bb_write (in):          write=6 read=8
[1] bb_write (out):         write=7 read=8

[1] bb_write_current (in):  write=7 read=8
[1] bb_write_current (out): write=7 read=8
[1] bb_write (in):          write=7 read=8

  3: ROLS_LOOP LOCKed, waiting to send buffer ...
  1 SEND_BUFFER_ROC 3 (0x00000000)
  [1] bb_write_current (in):  write=7 read=8
  [1] bb_write_current (out): write=7 read=8
  coda_roc: calling 'bb_write', MAY WAIT INSIDE FOREVER IF THERE IS NO SPACE !!!!!!!!!!!!!!!!!!!
  [1] bb_write (in):          write=7 read=8

setHeartError: 0 >sys 0, mask 22<
WARN: HeartBeat[0]: heartbeat=1469225947(1469225947) heartmask=22
UDP_cancel: cancel >inf:adcecal5 sys 0, mask 22<

		*/



        /* 'delayed' done */
        if(rolP->doDone)
        {
          /*printf("ROLS_LOOP calls __done() (no space in buffer for the next event ?)\n");fflush(stdout);*/
          /* "__done()" routine have to be called to enable next trigger;
          we are here if it was not called by 'WRITE_EVENT_'; it happened
          because it was no space in buffer for the next event */
          /* here we wait for buffer become available, and call "__done()" ourself */
          SENDBUFFER_LOCK;
          printf("3: ROLS_LOOP LOCKed, waiting to send buffer ...\n");fflush(stdout);
          SEND_BUFFER_(3);
          printf("3: ROLS_LOOP LOCKed, buffer sent\n");fflush(stdout);
          SENDBUFFER_UNLOCK;
          printf("3: ROLS_LOOP UNLOCKed\n");fflush(stdout);
		  /* call done only if writing was successful !!!???*/
          setHeartBeat(HB_ROL,23,5);
TRANSITION_LOCK;
          rolP->daproc = DA_DONE_PROC;
          rolP->doDone = 0;
          (*rolP->rol_code) (rolP);
TRANSITION_UNLOCK;
          setHeartBeat(HB_ROL,24,5);
		}
      }

      setHeartBeat(HB_ROL,25,5);

      /* end of ROL1 */
	  /***************/

      /* update statistics: 'object_nlongs' updated by ROL1, we just copying to old location*/
	  object->nlongs = object_nlongs;







	  /* need following to send buffer on 'timeout' for low datarate crates, otherwise it will never send anything
		 because high datarate crates will fill EB buffers and ... ???????? */

      /*if(rocp->state == DA_ENDING)*/
      {
        /* check for timeout - NEED TO SYNCHRONIZE WITH ROL1 !!! */
        /* on timeout condition we are sending ONLY if buffer is not empty - this is what we want ? */
        currenttime = gethrtime();
        if(currenttime > timeout)
        {
/*prints all the time
		  printf("coda_roc: currenttime=%lld timeout=%lld dataInBuf=%d\n",currenttime,timeout,dataInBuf);fflush(stdout);
*/
          if(dataInBuf > BBHEAD_BYTES)
          {
/*prints all the time
		    printf("coda_roc: ..sent ! dataInBuf=%d BBHEAD_BYTES=%d\n",dataInBuf,BBHEAD_BYTES);
*/
            clear_to_send  = 1;
            SENDBUFFER_LOCK;
/*prints all the time
            printf("4: ROLS_LOOP LOCKed, waiting to send buffer ...\n");fflush(stdout);
*/
            SEND_BUFFER_(4);
/*prints all the time
            printf("4: ROLS_LOOP LOCKed, buffer sent\n");fflush(stdout);
*/
            SENDBUFFER_UNLOCK;
/*prints all the time
            printf("4: ROLS_LOOP UNLOCKed\n");fflush(stdout);
*/
          }
          else
		  {
            SET_TIMEOUT(89);
		  }
/*prints all the time
          printf(" ..break, timeout=%lld\n",timeout);fflush(stdout);
*/
          break;
        }
        setHeartBeat(HB_ROL,26,5);
      }






    } while(1); /* 'do' loop */
    /**********************************************/
    /**********************************************/
    /**********************************************/

    setHeartBeat(HB_ROL,7,1);

#ifndef Darwin
    if(nevent!=0)
    {
      icycle ++;
      if(icycle>=cycle)
      {
        printf("rols_thread: waiting=%7llu processing=%7llu microsec per event (nev=%d)\n",
          time1/nevent,time2/nevent,nevent);
        icycle = 0;
      }
      nevent = time1 = time2 = 0;
    }
    else
    {
      /*printf("rols_thread: nevent==0 !!! (end=???) %d %d\n")*/;
    } 
#endif

    setHeartBeat(HB_ROL,8,5);

  } /* if(rocp->active > 1) */


  /********************/
  /********************/
  /*SENDING big buffer - old location*/
  /*
  SEND_BUFFER_(9);
  */
  /********************/
  /********************/

  /************************/
  /* Handle state changes */
  setHeartBeat(HB_ROL,9,5);
  switch(rocp->state)
  {
  case DA_DOWNLOADED: 
    /*printf("DA_DOWNLOADED\n");*/
    sleep(1);
    TRANSITION_LOCK;
	/* ERRORRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR */
    setHeartBeat(HB_ROL,10,5);
    if((rocp->output_switch == 0) && (bignet.doclose == 1))
    {
      /* printf("delete \n"); */
      printf("call: '%s close_links'\n", object->name);
      rocCloseLink();

      bignet.doclose = 0;
    }
    TRANSITION_UNLOCK;
    break;

  case DA_PRESTARTING:
    /*do nothing, we just prestarting*/
    break;

  case DA_PRESTARTED:
    /*do nothing, we just prestarted and are waiting for 'Go'*/
    break;

  case DA_ACTIVE:
    /*if we are active then all we need to do is pass data to EB*/
    break;

  case DA_PAUSING:
    printf("DA_PAUSING\n");
    sleep(1);
    TRANSITION_LOCK;
    /*
     * If we got this far then either the last event is either on
     * the iov or was the last event on the last iov.
     */
    setHeartBeat(HB_ROL,11,5);
    if(rocp->active != 1)
    {
      if(rocp->async_roc_flag == 0)
      {
        /*ERROR??? can be a problem if ROL1 still filling big buffer ???*/
        informEB(object, (unsigned int) EV_PAUSE, (unsigned int) 0, (unsigned int) object->nevents);
      }
    }
    if(codaUpdateStatus("paused") != CODA_OK)
    {
      setHeartBeat(HB_ROL,0,-1);
      TRANSITION_UNLOCK;
      return;
	}
    tcpState = rocp->state = DA_PAUSED;
    /*
     * in pause we leave the partial iov hanging in the air until
     * someone hits go
     */
    rocp->active = 1;
    TRANSITION_UNLOCK;
    break;

  case DA_ENDING:
    printf("coda_roc: DA_ENDING before lock\n"); fflush(stdout);
    TRANSITION_LOCK;
    printf("coda_roc: DA_ENDING after lock\n"); fflush(stdout);
    setHeartBeat(HB_ROL,12,5);

    /* make sure the output buffer gets sent if there is any data in it */     
    if(dataInBuf > BBHEAD_BYTES)
    {
      clear_to_send = 1;
      printf("coda_roc: in DA_ENDING dataInBuf=%d BBHEAD_BYTES=%d\n",dataInBuf,BBHEAD_BYTES);
      TRANSITION_UNLOCK;
      break;
    }

    /* make sure we have the last event in the output buffer */
    if(/*rocp->last_event*/last_event_check < object->nevents)
    {
      printf("coda_roc: DA_ENDING last event=%d nevents=%d\n",
			 /*rocp->last_event*/last_event_check,object->nevents);
      TRANSITION_UNLOCK;
      break;
    }

	/* sergey: at that point rocp->last_event == object->nevents ???!!! */
    printf("coda_roc: in DA_ENDING last event=%d nevents=%d\n",
		   /*rocp->last_event*/last_event_check,object->nevents);
    printf("coda_roc: in DA_ENDING rocp->active=%d\n",rocp->active);

    /*if we got this far then either the last event sent was the last event taken
     OR the last event taken is sat at the end of the output queue waiting to go */
    if(rocp->active == 2)
    {
      if(rocp->async_roc_flag == 0)
      {
        printf("coda_roc: call informEB(EV_END)\n");
        informEB(object, (unsigned int) EV_END, (unsigned int) object->runNumber,
                 (unsigned int) object->nevents);
      }
      printf("Inserted End event on queue\n");
      rocp->active = 1;

	  /*sergey: should we wait here until 'roc_network::LINK_close' called by netwotk thread, so we close link before EB End transition called ? */
      /* as soon as our status become 'downloaded' runcontrol will send End command to EB (?) */
      if(codaUpdateStatus("downloaded") != CODA_OK)
      {
        setHeartBeat(HB_ROL,0,-1);
        TRANSITION_UNLOCK;
        return;
	  }
      tcpState = rocp->state = DA_DOWNLOADED;
      printf("ended after %d events\n",object->nevents);
      TRANSITION_UNLOCK;
      break;
    }

    TRANSITION_UNLOCK;
    break;

  default:
    printf("default: rocp->state=%d, you better describe it in 'roc_component'\n",rocp->state);
    break;
  }
  setHeartBeat(HB_ROL,13,5);


  return;
}


/* Recover_Init() must be called for following to work !!! (see coda_constructor.c) */
__attribute__((destructor)) void end (void)
{
  printf("__attribute__\n");fflush(stdout);
#ifdef Linux_vme
  printf("coda_roc is exiting, clear dma memory\n");
  bb_dma_free();

  printf("\n\n coda_roc: ======= Close the default VME windows =========\n\n");
  /*vmeCloseA32Slave();*/
  vmeCloseDefaultWindows();
#endif
}



/****************/
/* main program */
/****************/


void
main (int argc, char **argv)
{
  CODA_Init (argc, argv);

  roc_constructor();

  printf("main 1\n");
  CODA_Execute ();
  printf("main 2\n");


  
}
