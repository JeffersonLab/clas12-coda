
/* coda_ebc.c */

#include <stdio.h>


#if defined(Linux_armv7l)

int
main()
{
  printf("coda_eb is dummy for ARM etc\n");
}

#else


//#define DEBUG

#define MAX_NODES 300


/* INCLUDES */

#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/mman.h>

#include "rc.h"
#include "rolInt.h"
#include "da.h"
#include "circbuf.h"
#include "libdb.h"

#include "LINK_support.h"
#include "CODA_format.h"


/************/
/* ET stuff */

#include "et_private.h"
extern char	et_name[ET_FILENAME_LENGTH];
static et_sys_id	et_sys;
static int		et_locality;
size_t          et_eventsize;
static int		et_init = 0, et_reinit = 0;

/*static*/ char confFile[256];

void *handle_build();

/* NOTE: nthreads*chunk MUST BE LESS then total number of events in ET */
static int nthreads = 1;
static int chunk = 100; /* 100; MUST BE LESS THEN NCHUNKMAX !!! */
static int ievent_old;

/* defined in et_private.h
#define CODA_ERROR 1
#define CODA_OK 0
*/

#define SOFT_TRIG_FIX_EB \
  /*printf("befor: typ=%d\n",typ);*/ \
  /*if(typ==253) typ=0x41;*/		 \
  /*if(typ==254) typ=0x42;*/		 \
  /*printf("after: typ=%d\n",typ)*/

/*allocate it dynamically in according to ET event size
#define MAXBUF 100000
static unsigned int hpsbuf[MAXBUF];
*/

static unsigned int MAXBUF;
static unsigned int *hpsbuf = NULL;


#define EVIO_RECORD_HEADER(myptr, mylen) \
  myptr[0] = mylen + 8; /* Block Size including 8 words header + mylen event size  */ \
  myptr[1] = 1; /* block number */ \
  myptr[2] = 8; /* Header Length = 8 (EV_HDSIZ) */ \
  myptr[3] = 1; /* event count (1 in our case) */ \
  myptr[4] = 0; /* Reserved */ \
  myptr[5] = 0x204; /*evio version (10th bit indicates last block ?)*/ \
  myptr[6] = 1; /* Reserved */ \
  myptr[7] = 0xc0da0100; /* EV_MAGIC */


/* event already in ET system; get it back, create 'correct' record in ET, and put event into that 'correct' record */
#define HPS_HACK \
  for(ii=0; ii<nevents2put; ii++) \
  { \
    int jjj, status, handle1; \
    size_t size; \
    unsigned int *ptr; \
    ptr = (unsigned int *)etevents[ii]->pdata; \
    et_event_getlength(etevents[ii], &size); /*get event length from et*/ \
    len = size; \
    /*printf("[%3d] len1=%d\n",ii,len);*/ \
    memcpy((char *)hpsbuf, (char *)ptr, len); /*copy event from et to the temporary buffer*/ \
    /*for(jjj=0; jjj<(len/4); jjj++) printf("HACK [%3d] 0x%08x\n",jjj,ptr[jjj]);*/ \
    EVIO_RECORD_HEADER(ptr,(len/4));									\
    memcpy((char *)(ptr+8), (char *)hpsbuf, len); \
    len += 32;  /* add 8 words for record header we just created, and update et event length */ \
	/*printf("len2=%d\n",len/4);*/ \
    et_event_setlength(etevents[ii], len); /*update event length in et*/ /*170000us*/ \
    /*printf("[%3d] len3=%d\n",ii,len);*/ \
  }


#define HPS_HACK_OLD \
  for(ii=0; ii<nevents2put; ii++) \
  { \
    int jjj, status, handle1;					\
    unsigned int *ptr; \
    ptr = (unsigned int *)etevents[ii]->pdata; \
    et_event_getlength(etevents[ii], &len); /*get event length from et*/ \
    printf("[%3d] len1=%d\n",ii,len); 							\
    memcpy((char *)hpsbuf, (char *)etevents[ii]->pdata, len); /*copy event from et to the temporary buffer*/ \
    for(jjj=0; jjj<(len/4); jjj++) printf("HACK [%3d] 0x%08x\n",jjj,ptr[jjj]); \
    status = evOpenBuffer(etevents[ii]->pdata, MAXBUF, "w", &handle1); /*open 'buffer' in et*/ \
    if(status!=0) printf("evOpenBuffer returns %d\n",status); \
    status = evWrite(handle1, hpsbuf); /*write event to the 'buffer'*/ \
    if(status!=0) printf("evWrite returns %d\n",status); \
    evGetBufferLength(handle1,&len); /*get 'buffer' length*/ \
	printf("len2=%d\n",len/4); 						 \
    status = evClose(handle1); /*close 'buffer'*/ \
    if(status!=0) printf("evClose returns %d\n",status); \
    et_event_setlength(etevents[ii], len); /*update event length in et*/ \
  }





/*static*/ extern objClass localobject;
extern char    *mysql_host; /* coda_component.c */
extern char    *expid; /* coda_component.c */
extern char    *session; /* coda_component.c */

static int ended_loop_exit = 0;
static int first_roc_id; /*for debugging purposes - contains first arrived roc_id in every event*/
static int remember_roc_ids[256];
static int remember_event_numbers[256];
static int remember_nrocs_so_far;


int listSplit1(char *list, int flag, int *argc, char argv[LISTARGV1][LISTARGV2]);



/* used by debopenlinks() and debcloselinks() */
static int  linkArgc;
static char linkArgv[LISTARGV1][LISTARGV2];

DATA_LINK debOpenLink(char *fromname, char *toname, char *tohost, MYSQL *dbsock);
int debCloseLink(DATA_LINK theLink, MYSQL *dbsock);






/* queues for events ordering */
static int id_in_index, id_out_index;
static int id_in[NIDMAX];
static int id_out[NIDMAX];

static pthread_mutex_t id_out_lock; /* lock the 'idout' */
static pthread_cond_t id_out_empty; /* condition for 'idout' */

#define out_lock    pthread_mutex_lock(&id_out_lock)
#define out_unlock  pthread_mutex_unlock(&id_out_lock)










/* ??? need to lock getting data from ET if nthreads>1 */

/*
#define data_lock(ebp_m)  pthread_mutex_lock(&(ebp_m)->data_mutex)
#define data_unlock(ebp_m)  pthread_mutex_unlock(&(ebp_m)->data_mutex)
*/
#define data_lock(ebp_m) ;
#define data_unlock(ebp_m) ;






extern WORD128 roc_linked; /* linked ROCs mask (see LINK_support.c)*/
extern CIRCBUF *roc_queues[MAX_ROCS]; /* see LINK_support.c */
extern int roc_queue_ix; /* cleaned up here, increment in LINK_support.c */
extern unsigned int *bufpool[MAX_ROCS][QSIZE];  /* see LINK_support.c */

static unsigned int *evptr[MAX_ROCS][NCHUNKMAX];

extern char configname[128]; /* coda_component.c */


/* param struct for building thread */
typedef struct thread_args *ebArg;
typedef struct thread_args
{
  objClass object;
  int *interp_obsolete;
  int *thread_exit;
  int id;
} EBARGS;


typedef struct EBpriv *EBp;
typedef struct EBpriv
{
  int nrocs;
  int roc_id[MAX_ROCS];
  int roc_nb[MAX_ROCS];
  /*
  CIRCBUF **roc_stream[MAX_ROCS];
  */

  /*int active;*/ /* obsolete ? */

  int ended;
  int ending;
  int end_event_done;
  int force_end;

  /*int *interp_obsolete;*/

  /*pthread_mutex_t active_mutex;*/ /* obsolete ? */
  /*pthread_cond_t active_cond;*/ /* obsolete ? */

  pthread_mutex_t data_mutex;
  pthread_cond_t data_cond;

  int nthreads;               /* the number of building threads */
  pthread_t idth[NTHREADMAX]; /* pointers to building threads */

  WORD128 roc_mask;
  WORD128 ctl_mask;

  unsigned int cur_cntl;

  void *out_id[NTHREADMAX];
  char out_name[200];

  /* pointers to the link structures */
  DATA_LINK links[MAX_ROCS];

} eb_priv;

typedef struct bank_part *bankPART;
typedef struct bank_part
{
  int bank;
  int length;
  unsigned int *data;
  evDesc desc;
} BANKPART;

typedef int (*IFUNCPTR) ();

/* local functions */
static int debcloselinks();

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




/* ET Initialization */    
int
eb_et_initialize(void)
{
  et_openconfig   openconfig;
  struct timespec timeout;
  int    events_total;

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

  if(et_open_config_init(&openconfig) != ET_OK)
  {
    printf("deb ET init: cannot allocate mem to open ET system\n");
    return CODA_ERROR;
  }
  et_open_config_setwait(openconfig, ET_OPEN_WAIT);
  et_open_config_settimeout(openconfig, timeout);
  if(et_open(&et_sys, et_name, openconfig) != ET_OK)
  {
    printf("deb ET init: cannot open ET system\n");
    return CODA_ERROR;
  }
  et_open_config_destroy(openconfig);

  /* set level of debug output */
  et_system_setdebug(et_sys, ET_DEBUG_ERROR);

  /* where am I relative to the ET system? */
  et_system_getlocality(et_sys, &et_locality);

  /* find out how many events in this ET system */
  if(et_system_getnumevents(et_sys, &events_total) != ET_OK)
  {
    et_close(et_sys);
    printf("deb ET init: can't find # events in ET system\n");
    return(CODA_ERROR);
  }

  /* find out event size in this ET system */
  if(et_system_geteventsize(et_sys, &et_eventsize) != ET_OK)
  {
    et_close(et_sys);
    printf("deb ET init: can't find event size in ET system\n");
    return(CODA_ERROR);
  }
  else
  {
    printf("ET init: event size %d bytes\n",et_eventsize);
  }

  et_init++;
  et_reinit = 0;
  printf("deb ET init: ET fully initialized\n");

  return(CODA_OK);
}

/* end of ET stuff */
/*******************/






/*Sergey: temporary just for test
int
tmpUpdateStatistics()
{
  char tmp[1000];

  sprintf(tmp,"ts2: %d %d %d %d %d %d %d %d %d %d %d %d %d ",77,
    1,2,3,4,5,6,7,8,9,10,11,12);

  UDP_request(tmp);

  return(0);
}
*/










/********************************************************************
 handle_build : This is the main routine of the "Build thread" it is
                executed as a detached thread.
*********************************************************************/

#define NPROF1 100
#define NPROF2 100

void *
handle_build(ebArg arg)
{
  objClass object;
  DATA_DESC descriptors[MAX_ROCS];
  BANKPART *build_node[MAX_NODES];
  BANKPART build_nodes[MAX_NODES];
  EBp ebp;
  int ii, len, id, in_error = 0, res = 0, i, j, k, ix, node_ix, cc, roc, lenbuf, rocid;
  DATA_DESC *desc1, desc2;
  int types[MAX_ROCS];

  WORD128 fragment_mask;
  WORD128 sync_mask;
  WORD128 type_mask;

  int current_evnb;
  int current_evty;
  int current_sync;
  unsigned int current_syncerr, current_evtyerr;
  int nevents2put=0, neventsfree=0, neventsnew = 0;
  int idin, idout, itmp;
  int print_rocs_report = 1;

  int nevbuf, nevrem;
  int nphys;

  hrtime_t start1, end1, start2, end2, time1=0, time2=0, time3=0;
  hrtime_t nevtime1=0, nevtime2=0, nevchun=0;

  int status;
  et_event *cevent = NULL;
  et_att_id	et_attach;
  et_event **etevents = NULL;
  int *ethandles = NULL;
  int ievent;

  unsigned int total_length; /* full event length in bytes */



  object = arg->object;
  id = arg->id;
  ebp = (void *) object->privated;


  printf("[%1d] handle_build starting ..\n",id); fflush(stdout);

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &ix);
  pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, &ix);


  /* attachment */
  if(et_station_attach(et_sys, ET_GRANDCENTRAL, &et_attach) < 0)
  {
    et_close(et_sys);
    printf("[%1d] cannot attach to ET station - exit.\n",id);
    exit(0);
  }
  else
  {
    printf("[%1d] attached to ET\n",id);
  }

  /* allocate memory for events from ET */
  if((etevents = (et_event **) calloc(NCHUNKMAX, sizeof(et_event *))) == NULL)
  {
    et_close(et_sys);
    printf("[%1d] deb ET init: no mem left for etevents\n",id);
    exit(0);
  }

  /* allocate memory for ET buffer handles*/
  if((ethandles = (int *) calloc(NCHUNKMAX, sizeof(int))) == NULL)
  {
    et_close(et_sys);
    printf("[%1d] deb ET init: no mem left for ethandles\n",id);
    exit(0);
  }



  /* sergey: allocate memory for event chunks; will be filled by cb_events_get(), see macro FILL_EVENT_PTR_ARRAY 
  for(i=0; i<MAX_ROCS; i++)
  {
    for(j=0; j<NCHUNKMAX; j++)
	{
      evptr[i][j] = malloc(et_eventsize+512);
      if(evptr[i][j]==NULL)
	  {
        printf("ERROR: cannot allocate memory for evptr[%2d][%3d] - exit\n",i,j); fflush(stdout);
        exit(0);
	  }
      else
	  {
        printf("Allocated %d bytes for evptr[%2d][%3d]\n",et_eventsize+512,i,j);
	  }
	}
  }
*/


  /* some initialization */
  for(i=0; i<MAX_ROCS; i++) types[i] = -1;
  for(i=0; i<MAX_NODES; i++)
  {
    build_node[i] = &build_nodes[i];
    bzero((void *)build_node[i],sizeof(BANKPART));
  }

  Clear128(&fragment_mask);
  Clear128(&sync_mask);
  Clear128(&type_mask);

  current_evty = -1;
  current_evnb = -1;
  current_sync =  0;
  current_syncerr = 0;
  current_evtyerr = 0;
  total_length = 0;
  node_ix = 0;

  /* desc2 initialization */
  desc2.time_ = time(NULL);
  desc2.runty = object->runType;
  desc2.runnb = object->runNumber;

  /* start out with nothing in descriptors */
  bzero((void *)descriptors,sizeof(descriptors));






  /*******************
  * MAIN LOOP STARTS *
  *******************/

  nevbuf = 0;
  do
  {
    int bank, typ, issync;
    unsigned int *soe, *dabufp, *data, *temp;
    WORD128 roc_mask = ebp->roc_mask;
    unsigned int indx;
    BANKPART *cur_val;
    char *ctype, *dtype;

    typ = -1; /* Sergey: to make debug messages happy .. */



top: /* 'force_end' entry */

	/*
start2 = gethrtime();
	*/

    /* check if we need to end in a hurry (force_end);
    in that case we have to check if "end event" was already written */
    if(ebp->force_end)
    {
      printf("[%1d] force_end %d end_event_done %d\n",id,ebp->force_end,ebp->end_event_done);
      Print128(&ebp->roc_mask);
      fflush(stdout);

      if(ebp->end_event_done)
      {
        printf("[%1d] we are ended already, goto output_event\n",id);
        fflush(stdout);
        goto output_event;
      }

      printf("[%1d] NEED TO TEST AND PROBABLY REDESIGN THAT PART !!!\n",id);
      fflush(stdout);

      /* if there is no empty ET buffers go and get it */
      /* if ET system is dead or can't get event, don't put it */
      if(neventsfree < 1)
      {
        nevents2put = 0;
        neventsfree = 1;
        in_error = et_event_new(et_sys,et_attach,&etevents[0],ET_SLEEP,NULL,120);
        if(in_error != ET_OK)
        {
          /* need to reinit ET system since something's wrong */
          printf("[%1d] ERROR: et_event_new() returns %d\n",id,in_error);
          et_reinit = 1;
          break;
        }
        else
        {
          printf("[%1d] et_event_new() 1\n",id);
        }
      }


      soe = dabufp = (unsigned int *) etevents[nevents2put]->pdata;


      nevents2put++;
      neventsfree--;

      desc2.type = EV_END;
      desc2.time_ = time(NULL);
      desc2.rocs[0] = 0;
      desc2.evnb = object->nevents;
      desc2.runty = object->runType;
      desc2.runnb = object->runNumber;

printf("[%1d] EVENT_ENCODE_SPEC 2 ..\n",id);fflush(stdout);
      CODA_encode_spec(&dabufp, &desc2);
printf("[%1d] .. done\n",id);fflush(stdout);

      ebp->ended = 1;
      /*ebp->end_event_done = 1; ??? */
      bzero((void *) descriptors, sizeof(descriptors));
      node_ix = 0;

      /* Fix total_length for Control Events */
      total_length = desc2.length;

      goto output_event;
    }








    /* check if all ROCs are linked (roc_linked must be filled up
       by LINK_sized_read (LINK_support.c file)) */
    /* usually have to wait in Prestart */

    while( (EQ128(&roc_mask,&roc_linked)==0) && (ebp->ended==0) && (ebp->ending==0))
    {
      /*roc_linked hungs sometimes ..*/
      printf("NEED: ");
      Print128(&roc_mask);
      printf("HAVE: ");
      Print128(&roc_linked);
      printf("[%1d] waiting for the following ROC IDs:\n",arg->id);
      itmp = 1;
      for(i=0; i<MAX_ROCS; i++)
      {
        if( CheckBit128(&roc_mask,i) != 0 && CheckBit128(&roc_linked,i) == 0 ) {printf(" %2d",i);fflush(stdout);}
      }
      printf("\n\n");
      fflush(stdout);
      sleep(3);

      if(ebp->force_end)
      {
        printf("Stop waiting for the rocs because of force_end condition\n");
        goto top;
      }

    }

    if(print_rocs_report)
    {
      printf("NEEDED:\n");
      Print128(&roc_mask);
      printf("RECEIVED:\n");
      Print128(&roc_linked);
      printf("[%1d] all ROCs reported\n",arg->id);
      fflush(stdout);
      print_rocs_report = 0;
	}







    /******************
    * ROC LOOP STARTS *
    ******************/

retry:


    /************************************************/
    /* if buffer is empty get a new chunk of events */
    if(nevbuf == 0)
    {



#ifdef DO_NOT_PUT
      printf("EB sleeps forever, all data go to trash !\n");
      while(1) sleep(10); /* sleep forever */
#endif


data_lock(ebp);

      start1 = gethrtime();


      /* get chunk of events from roc fifos */
#ifdef DO_NOT_BUILD
      while(1)
	  {
#endif
        nevbuf = cb_events_get(roc_queues, id, ebp->nrocs, chunk, evptr, &nphys);
#ifdef DO_NOT_BUILD
	    /*printf("================> got %d events\n",nevbuf);*/
	  }
#endif



      idin = id_in[id_in_index];
      id_in_index = (id_in_index + 1) % NIDMAX;

      end1 = gethrtime();

/*
printf("[%1d] .. got idin=%d, nevbuf=%d, nphys=%d, event no.=%d\n",
id,idin,nevbuf,nphys,evptr[0][0][1]&0xff);
*/

      time1 += ((end1-start1)/NANOMICRO);
      time3 -= ((end1-start1)/NANOMICRO);
      nevchun = nevchun + ((hrtime_t)nevbuf);
      if(++nevtime1 == NPROF1)
      {
        /*
        printf("2: time1=%7lld microsec, chuck=%7lld",
          time1/nevtime1,nevchun/nevtime1);
        if(nevchun > 0) printf(" -> %7lld microsec per event\n",time1/nevchun);
        else printf("\n");
        */
        nevtime1 = 0;
        time1 = 0;
        nevchun = 0;
      }



      if(nevbuf == -1)
      {
        /*ebp->roc_mask &= ~ (1<<roc); was ERROR even for 32-bit: 'roc' is not defined here !!! */
        printf("handle_build: cb_events_get() returned end of file\n");
        fflush(stdout);
        ebp->force_end = 2;
        data_unlock(ebp);
        goto top;
      }
      else if(nevbuf < 0)
      {
        printf("ERROR: nevbuf=%d\n",nevbuf);
        fflush(stdout);
      }


data_unlock(ebp);









      /*********************************************************/
      /* if we've run out of empty ET events, get more from ET */
      if(neventsfree < 1)
      {
        neventsnew = 0;
        nevrem = nevbuf;
        while(nevrem)
        {
          in_error = et_events_new(et_sys, et_attach, &etevents[neventsnew],
                                   ET_SLEEP, NULL, et_eventsize, nevrem, &cc);
          if(in_error != ET_OK)
          {
            printf("et_events_new returns ERROR = %d\n",in_error); fflush(stdout);
            ebp->force_end = 4;
            et_reinit = 1;
            break;
          }
          neventsnew += cc;
          nevrem -= cc;
        }
        neventsfree = neventsnew;
        if(nevbuf!=neventsnew)
        {
          printf("deb_component ERROR: nevbuf=%d != neventsnew=%d\n",
                 nevbuf,neventsnew);
          fflush(stdout);
          exit(0);
        }
      }


      /* store some informations into roc descriptors */
      for(roc=0; roc<ebp->nrocs; roc++)
      {
        desc1 = &descriptors[roc];

        desc1->rocid = roc_queues[roc]->rocid;
        if(desc1->rocid < 0 || desc1->rocid >= MAX_ROCS)
        {
          printf("ERROR: rocid=%d\n",desc1->rocid);
          fflush(stdout);
	  exit(0);
        }

        desc1->soe = (unsigned int *)evptr[roc][0]; /* start of first event */

        desc1->nevbuf = nevbuf;
        desc1->totbuf = desc1->nevbuf; /* remember total number of events */
        desc1->user[0] = 0;
        desc1->ievbuf = 0;
      }


    }
    /* end of getting new chunk of events */
    /**************************************/







    /*******************/
    /* for() over rocs */
    for(roc=0; roc<ebp->nrocs; roc++)
    {
      desc1 = &descriptors[roc];
		/*
printf("!!!coda_ebc: desc1=0x%08x\n",desc1);
		*/

      /* Decode the event; returns with 'temp' pointing to the first data
      word AFTER the header, note also that if there is no data from this ROC
      EVENT_DECODE_FRAG should still behave itself and return a descriptor */
      temp = data = desc1->soe;
      /*printf("!!!coda_ebc: data=0x%08x\n",data);*/
      typ = (data[1] >> 16) & 0xff;       /* Temp storage of type info */
      SOFT_TRIG_FIX_EB;
      issync = (data[1] >> 24) & 0x01;    /* Temp storage of sync info */
      /*printf("== roc=%2d, typ=%3d(0x%4x), issync=%d\n",roc,typ,typ,issync);*/
      if( (typ < EV_SYNC) || (typ >= (EV_SYNC+16)) ) /* physics event */
      {
        CODA_decode_frag(&temp,desc1);

        /* extract some info from bank(s) and fill in user[] array (for example trigger bits) */
        et_user(temp, desc1->user);
		/*
printf("!!!coda_ebc: roc=%d desc1->evnb=%d\n",roc,desc1->evnb);
		*/
      }
      else if( (typ >= EV_SYNC) && (typ < (EV_SYNC+16)) ) /* special event */
      {
        CODA_decode_spec(&temp,desc1);

        desc1->evnb = -1;        /* to be sure */


        /*********************************************************************/
        /*********************************************************************/
        /*********************************************************************/
        /* following is a temporary solution for ending problem; current
        algorithm works only if all 'End' events have the same number so
        they are coming together; that behaviour inforced in ROCs and it is
        true for normal running conditions, but it is not guaranteed, and
        sometimes we have a problem if 'End' event from one ROC not in a
        sync with 'End' events from other ROC(s); temporary solution is that
        we are setting 'force_end=1' and going to 'top' where force end
        situation will be handled; what probably should be done instead: if
        we obtained first 'End' event from some ROC, loop over other ROCs
        and for every ROC skip non-End events until 'End' event obtained */

        if(typ==EV_END)
        {
          if(ebp->ending==1)
          {
            printf("[%1d] WE ARE ENDING AND GOT END EVENT FROM ROC %d\n",
              id,roc);
          }
          else
          {
            printf("[%1d] WE ARE NOT ENDING BUT GOT END EVENT FROM ROC %d\n",
              id,roc);
          }

          ebp->force_end = 1;
          goto top;

        }

        /*********************************************************************/
        /*********************************************************************/
        /*********************************************************************/

        printf("handle_build: roc %d .. done.\n",roc); fflush(stdout);
      }
      else
      {
        /* should never come here */
        printf("[%1d] Event type=%d is NOT supported - exit\n",id,typ);
        fflush(stdout);
        exit(0);
      }


      /* sequence error check */
      if(current_evty == -1) /* event type -1 is used it to mark new event */
      {
        current_evty = typ;
        current_evnb = desc1->evnb;




        ievent = current_evnb;

	/* ievent can be -1 for Prestart event for example (?), we are checking only Physics events */
	if(ievent>=0)
	{
	  /*event number must be incremented by 1*/
	  //printf(">>> ievent=%d, ievent_old=%d\n",ievent,ievent_old);
          if(ievent_old==255) ievent_old = -1;
          if(ievent!=(ievent_old+1)) printf("ERROR: ievent_old=%d, ievent=%d\n",ievent_old,ievent);
          ievent_old = ievent;
	}




        first_roc_id = desc1->rocid;

remember_roc_ids[0] = desc1->rocid;
remember_event_numbers[0] = desc1->evnb;
remember_nrocs_so_far = 1;


#ifdef DEBUG
        printf("[%1d] INFO: Event (Num %d type %d) from FIRST rocid=%d\n",
			   id,current_evnb,current_evty,desc1->rocid);
#endif
      }
      else
      {
#ifdef DEBUG
        printf("[%1d] INFO: Event (Num %d type %d) from rocid=%d\n",
			   id,desc1->evnb,typ,desc1->rocid);
#endif


remember_roc_ids[remember_nrocs_so_far] = desc1->rocid;
remember_event_numbers[remember_nrocs_so_far] = desc1->evnb;
remember_nrocs_so_far++;



        /* we are already building so we can check some things... */
        if((current_evnb != desc1->evnb) && (in_error == 0))
        {
          in_error = 1;
          printf("[%1d] FATAL: Event (Num %d type %d) NUMBER mismatch",
                 id,current_evnb,current_evty);
          fflush(stdout);
          printf(" -- %s (rocid %d) sent %d (type %d) - first reported roc_id was %d\n",
                  (get_cb_name(&roc_queues[ebp->roc_nb[desc1->rocid]])), 
		 desc1->rocid,desc1->evnb,desc1->type,first_roc_id);
          fflush(stdout);
          printf("[%1d] ERROR: Discard data until next control event\n",id);
          fflush(stdout);
          /*fragment_mask = 0;*/



printf("\n\nHAVE SO FAR FOR THE CURRENT EVENT:\n");
for(i=0; i<remember_nrocs_so_far; i++)
{
  printf("roc_id = %2d, event = %6d\n",remember_roc_ids[i],remember_event_numbers[i]);
}
printf("\n");









exit(0);
        }
        else if((current_evty != desc1->type) && (in_error == 0))
        {
#ifdef DEBUG
	  printf("[%1d] INFO: event type ???\n",id);
#endif
          current_evtyerr ++;   /* Count type mismatches from first fragment */

          /*type_mask |= (1<<desc1->rocid);*/ /* Get mask of ROC IDs with mismatched types */
          SetBit128(&type_mask,desc1->rocid);
                                            
          types[desc1->rocid] = desc1->type; /* Store type for each ROC */
        }
      }

      SetBit128(&fragment_mask,desc1->rocid);
      if(issync) SetBit128(&sync_mask,desc1->rocid);

      /* if there was no data from this ROC we need worry no more;
      this next "if" takes care of the situation where there was a
      CODA header from a ROC but no actual data */
      if(desc1->length > 0)
      {
        BANKPART *bn;

        ix=0;
        bn = build_node[node_ix];

        bn->data = desc1->fragments[ix];
        bn->bank = desc1->bankTag[ix];
        bn->length = desc1->fragLen[ix];
        total_length += bn->length;
        /*printf("[%3d] bn->length=%d\n",node_ix,bn->length);*/
        bn->desc = desc1;	      

        node_ix++;
      }






    }
    /* end of for() over rocs */
    /**************************/





    /****************
    * ROC LOOP ENDS *
    ****************/










    /* if we get here we have one fragment for
       each ROC and fragment_mask == ebp->roc_mask */

#if 1
    /***********************************/
    /* check for Event Type mismatches */
    if(current_evtyerr)
    {
	  
      printf("ERROR: Event (Num %d) TYPE mismatch -- %d ROC Banks\n",
              current_evnb, current_evtyerr); fflush(stdout);

      printf("  (ID Mask: ");
      Print128(&type_mask);

      printf("  differ from selected build type = %d\n",current_evty); fflush(stdout);

      printf("  Event Type Mismatch info:\n"); fflush(stdout);
      for(ix=0; ix<MAX_ROCS; ix++)
      {
	//printf("ebp->roc_id[%d]=%d\n",ix,ebp->roc_id[ix]);
        if(ebp->roc_id[ix]>=0)
	{
          if( CheckBit128(&type_mask,ebp->roc_id[ix]) != 0)
          {
            printf("    ROC ID = %d   Type = %d\n",ebp->roc_id[ix],types[ebp->roc_id[ix]]);
            fflush(stdout);
          }
	}
      }
	  
    }
#endif


    /*****************************/
    /* check for SYNC mismatches */
    if(! IFZERO128(&sync_mask)) /* if not zero */
    {
      current_sync = 1;
      if(! EQ128(&sync_mask,&fragment_mask)) /* if they are different */
      {
        printf("ERROR: Event (Num %d type %d) SYNC mismatch\n",
               current_evnb, current_evty); fflush(stdout);

      printf("    ROC mask: ");
      Print128(&fragment_mask);

      printf("   SYNC mask: ");
      Print128(&sync_mask);

      printf("[%1d] following ROC IDs seems out of sync:\n",arg->id);
      for(i=0; i<MAX_ROCS; i++)
      {
        if(ebp->roc_id[i]>=0)
	{
          /* i -> ebp->roc_id[i] ??? */
          if( CheckBit128(&fragment_mask,i) != 0 && CheckBit128(&sync_mask,i) == 0 ) printf(" %2d",i);
	}
      }
      printf("\n\n");

      


      fflush(stdout);



        /* keep info on which ROCs missed the Sync Event 
        current_syncerr = fragment_mask&(~sync_mask);
*/
      }
    }

    /* cleanup ... and reserve space */
    Clear128(&fragment_mask);
    Clear128(&sync_mask);
    Clear128(&type_mask);


    /* then set 'soe' pointer and update event counters */
    soe = dabufp = (unsigned int *) etevents[nevents2put]->pdata;
    nevents2put++;
    neventsfree--;

    /* if physics event, reserve space for the event header and event 'ID bank' */
    if( (current_evty < EV_SYNC) || (current_evty >= (EV_SYNC+16)) )
    {
      CODA_reserv_head(&dabufp, &desc2);
	  total_length += (2*4); /* increment total_length */
      CODA_reserv_desc(&dabufp, &desc2);
	  total_length += (5*4); /* increment total_length */

      object->nevents++;
      desc2.evnb = object->nevents;
      desc2.type = current_evty;

      desc2.syncev = current_sync;
      desc2.err[1] = current_syncerr;
    }

    bank = -1;


    /********************************************/
    /* if physics event, check length and build */
    if( (current_evty < EV_SYNC) || (current_evty >= (EV_SYNC+16)) )
    {
      /* if event too big, ignore it */
      if(total_length > et_eventsize)
      {
        printf("ERROR: event too big (event length %d bytes > et system buffer size %d bytes) - exit\n",total_length,et_eventsize);
        fflush(stdout);
exit(0); /* should we ignore and try to recover ??? */
      }
      else
      {
        /* building */
		/*printf("\n Building, node_ix=%d\n",node_ix);*/
        for(indx=0; indx<node_ix; indx++) /* loop over sorted banks */
        {
          /*printf("indx=%d\n",indx);*/
          cur_val = build_node[indx];/* pointer to the next bank structure */
          desc1 = cur_val->desc;     /* descriptor for that bank */

          CODA_reserv_frag(&dabufp, &desc2); /* remember fragment starting address and bump 'dabufp' on 2 words */
          total_length += (2*4);

          bank = cur_val->bank;
          /*printf("bank=%d\n",bank);*/

          desc2.length = cur_val->length; /* full length in bytes */
          /*printf("desc2.length=%d words\n",desc2.length/4);*/

          memcpy((char *)dabufp, cur_val->data, cur_val->length); 
          /*for(i=0; i<cur_val->length/4; i++) printf("  [%3d] 0x%08x\n",i,dabufp[i]);*/
          dabufp += (cur_val->length >> 2);

          desc2.user[2] = desc1->user[2]; /*sergey: preserve contentType as it came from ROC (was always set to 1)*/
          desc2.user[3] = desc1->user[3]; /*sergey: preserve syncFlag as it came from ROC */

          desc2.rocid = bank;
          /*printf("desc2.rocid=%d\n",desc2.rocid);*/

          CODA_encode_frag(&dabufp, &desc2); /* fills fragment 2-word header */
        }
      }


    }
    else if( (current_evty >= EV_SYNC) && (current_evty < (EV_SYNC+16)) )      /* Control events - need no building */
    {
      printf("[%1d] Got control event fragments type = %d  node_ix=%d\n",id,current_evty,node_ix); fflush(stdout);
      for(indx=0; indx<node_ix; indx++)
      {
        cur_val = build_node[indx];
        desc1 = cur_val->desc;
        if(desc1)
        {
          /* handle control event; if 'End' set appropriate flags */

          if(desc1->type == EV_PRESTART)   ctype = "prestart";
          else if(desc1->type == EV_GO)    ctype = "go";
          else if(desc1->type == EV_PAUSE) ctype = "pause";
          else if(desc1->type == EV_END)   ctype = "end";  
          if( IFZERO128(&ebp->ctl_mask)==1 )
          {
            ebp->cur_cntl = desc1->type;
          }
          else
          {
            if(ebp->cur_cntl != desc1->type)
            {
              if(ebp->cur_cntl == EV_PRESTART)   dtype = "prestart";
              else if(ebp->cur_cntl == EV_GO)    dtype = "go";
              else if(ebp->cur_cntl == EV_PAUSE) dtype = "pause";
              else if(ebp->cur_cntl == EV_END)   dtype = "end";
              printf(", %s %s != %s ",(roc_queues[ebp->roc_nb[desc1->rocid]])->name,ctype,dtype);
            }
          }
          SetBit128(&ebp->ctl_mask,desc1->rocid);
          if( EQ128(&ebp->ctl_mask,&ebp->roc_mask) )
          {
            unsigned int *dabufp;
            printf(")\n-- Got all fragments of %s\n",ctype);
            Clear128(&ebp->ctl_mask);
            /* if thread did not get control event, we still need it !!! */
            desc1->time_ = time(NULL);

            /*desc1->rocs[0] = ebp->roc_mask; need it for 128 bit ???*/

            desc1->evnb = object->nevents;
            desc1->runty = object->runType;
            desc1->runnb = object->runNumber;

            dabufp = soe;

            CODA_encode_spec(&dabufp,desc1);

            if(desc1->type == EV_END)
            {
              ebp->ended = 1;
              ebp->end_event_done = 1;
            }
          }

          /* copy into desc2 */
          memcpy(&desc2,desc1,sizeof(DATA_DESC));
        }
      }
      /* Fix total_length for Control Events */
      total_length = desc1->length;
    }



    /* resync */
    node_ix = 0;
    if((current_evty < EV_SYNC) || (current_evty >= (EV_SYNC+16)) ) /* if physics event */
    {
      desc2.user[1] = 0;

      /* reserv for following 2 made in opposite order, so CODA_encode_head creates <event ..>
      and CODA_encode_desc creates <bank content="uint32" ...> */
      CODA_encode_desc(&dabufp, &desc2); /* fills event 'ID bank' reserved above by CODA_reserv_desc */
      CODA_encode_head(&dabufp, &desc2); /* fills event header reserved above by CODA_reserv_head */
    }


    /* go to the next event */
    for(roc=0; roc<ebp->nrocs; roc++)
    {
      desc1 = &descriptors[roc]; 
      /* if at least one event left in buffer goto the next event;
      if not - do nothing here, will get new buffer in the begining
      of the next itteration */
      if(desc1->nevbuf > 0)
      {
        desc1->nevbuf--; /* the number of events left in buffer */
        desc1->ievbuf++; /* current event number in buffer */
        desc1->user[0] = 0;

        /* get a pointer to the start of the next event; do not need
        to do that if(desc1->nevbuf==1) but 'if' will take more time */
        desc1->soe = (unsigned int *)evptr[roc][desc1->ievbuf];
      }
    }


    if(nevbuf > 0) nevbuf--;






output_event:

    if(ebp->end_event_done) printf("[%1d] output_event 1\n",id);fflush(stdout);

    /* if nothing wrong with ET system, output events */
    if(et_reinit == 0 && ebp->end_event_done == 0)
    {
      /* fill ET control words */

#ifdef RESTORE_OLD_SPEC_EVENT_CODING
	  /*printf("desc2.type befor = 0x%04x\n",desc2.type);*/
      if(desc2.type>=0x80) desc2.type = desc2.type & 0x7F; /*spec event - remove bit 7*/
      else desc2.type = desc2.type | 0x80 | desc2.user[3]; /*phys event - add bit 7, and sync bit*/
	  /*printf("desc2.type after = 0x%04x\n",desc2.type);*/
	  /*
	  if(desc2.user[3]) printf("coda_ebc: syncFlag detected (0x%08x), desc2.type = 0x%08x\n",
							   desc2.user[3],desc2.type);
	  */
#endif

      etevents[nevents2put-1]->control[0] = desc2.type;

      /*etevents[nevents2put-1]->control[1] = ebp->roc_mask; 128bit will not fit !!!*/

      etevents[nevents2put-1]->control[3] = desc2.user[1];

      etevents[nevents2put-1]->length     = total_length;

      /* put events back if no free events left or we're forced to end (NEED OUTPUT IF nevbuf==0 ???!!!) */
      if((neventsfree < 1) || ebp->force_end || ebp->ended)
      {
        /* put events into ET system */
        if(nevents2put > 0)
        {
          /* check if it is my turn */
          out_lock;
          {
            while(idin!=id_out[0])
            {
			  /*printf("[%1d]: cond_wait 1 ..\n",id);fflush(stdout);*/
              itmp = pthread_cond_wait(&id_out_empty, &id_out_lock);
			  /*printf("[%1d]: cond_wait 2 ..\n",id);fflush(stdout);*/
            }

            /*printf("[%1d]: put data idin=%d\n",id,idin);*/


start2 = gethrtime();

			HPS_HACK;

end2 = gethrtime();
time2 += ((end2-start2)/NANOMICRO);
if(++nevtime2 == NPROF2)
{
  printf("<<< time2=%7lld microsec (nevents2put=%d) >>>\n",time2/nevtime2,nevents2put);
  nevtime2 = 0;
  time2 = 0;
}


            in_error = et_events_put(et_sys,et_attach,etevents,nevents2put);

            j = id_out[0];
            for(i=0; i<NIDMAX-1; i++) id_out[i] = id_out[i+1];
            id_out[NIDMAX-1] = j;
          }
          pthread_cond_broadcast(&id_out_empty);
          out_unlock;

          if(in_error != ET_OK)
          {
            et_reinit = 1;
            ebp->force_end = 4;
          }
        }


        /* reset variables */
        nevents2put  = 0;

        if(neventsfree < 1) neventsfree = 0;
      }
    }
    if(ebp->end_event_done) printf("[%1d] output_event 2\n",id);fflush(stdout);

    current_evty = -1;
    current_sync = 0;
    current_syncerr = 0;
    current_evtyerr = 0;

    total_length = 0;


/*
end2 = gethrtime();
time2 += ((end2-start2)/NANOMICRO);
time3 += ((end2-start2)/NANOMICRO);
if(++nevtime2 == NPROF2)
{
  
  printf("<<< total=%7lld microsec, build=%7lld microsec >>>\n",
    time2/nevtime2,time3/nevtime2);
  
  nevtime2 = 0;
  time2 = 0;
  time3 = 0;
}
*/


  } while (!(ebp->force_end || ebp->ended));
  /*****************
  * MAIN LOOP ENDS *
  *****************/

































  /* lock and clean everything up */
/*data_lock(ebp);*/

  /************************/
  /* handle_build cleanup */
  /************************/
  printf("[%1d] handle_build_cleanup\n",arg->id);fflush(stdout);

  /* May still be events not written to ET system - flush; */
  /* that part is completely local, do not need to lock,   */
  /* we do not care about event ordering at that moment    */
  if(et_reinit == 0 && ebp->end_event_done == 0)
  {
    printf("[%1d] flush ET system\n",id);
    fflush(stdout);

    /* real events to be put into the system */
    if(nevents2put > 0)
    {
      HPS_HACK;
      et_events_put(et_sys, et_attach, etevents, nevents2put);
      printf("[%1d] put %d events to ET system\n",id,nevents2put);
    }
    /* new events left over after end event */
    if(neventsfree > 0)
    {
      et_events_dump(et_sys, et_attach, &etevents[(neventsnew-neventsfree)],
                     neventsfree);
      printf("[%1d] dump %d free events to ET system\n",id,neventsfree);
    }
  }
  free(etevents);

  /* detach from ET system */
  if(et_station_detach(et_sys, et_attach) < 0)
  {
    printf("[%1d] ERROR: cannot detach from ET station\n",id);
  }
  else
  {
    printf("[%1d] detached from ET\n",id);
  }


  /******************************************************/
  /* shutdown fifos; it is enough to do it by one thread,
  but lets all threads do it, it is not harm (is it ?) */
  printf("[%1d] remove mutex locks and shutdown fifos\n",arg->id);
  for(i=0; i<ebp->nrocs; i++)
  {
    CIRCBUF *f = roc_queues[i];
    f->deleting = 1;

    /* remove 'read' locks */
    if(pthread_mutex_trylock(&f->read_lock))
    {
      printf("[%1d] Mutex for %s was read-locked so unlock it\n",arg->id,get_cb_name(&f));
      fflush(stdout);
    }
    pthread_mutex_unlock(&f->read_lock);

    /* remove 'write' locks */
    if(pthread_mutex_trylock(&f->write_lock))
    {
      printf("[%1d] Mutex for %s was write-locked so unlock it\n",arg->id,get_cb_name(&f));
      fflush(stdout);
    }
    pthread_mutex_unlock(&f->write_lock);

    /* flush input streams */
    printf("\n[%1d] flushing input streams\n",arg->id);
    do
    {
      printf("[%1d] count for %s = %d\n",arg->id,get_cb_name(&f),get_cb_count(&f));
      fflush(stdout);
      if(get_cb_count(&f) <= 0) break;
      nevbuf = get_cb_data(&f, arg->id, chunk, evptr[i], &lenbuf, &rocid);
    } while(nevbuf != -1);
  }


  /* sergey: free memory for event chunks 
  for(i=0; i<MAX_ROCS; i++)
  {
    for(j=0; j<NCHUNKMAX; j++)
	{
      free(evptr[i][j]);
	}
  }
*/


  printf("[%1d] ============= Build threads cleaned\n",arg->id);
  printf("[%1d] ============= Build threads cleaned\n",arg->id);
  printf("[%1d] ============= Build threads cleaned\n",arg->id);
  printf("[%1d] ============= Build threads cleaned\n",arg->id);
  printf("[%1d] ============= Build threads cleaned\n",arg->id);
  printf("[%1d] build thread exiting: %d %d\n",
    id,ebp->force_end,ebp->ended); fflush(stdout);
  ebp->force_end = 0;
  roc_queue_ix = 0;

/*data_unlock(ebp);*/


  /* zero thread pointer; 'deb_end' will wait
  until all thread pointers are zero */
  ebp->idth[id] = 0;


  /* detach a thread */
  itmp = pthread_detach(pthread_self());
  if(itmp==0)
  {
    printf("[%1d] handle_build thread detached\n",id); fflush(stdout);  
  }
  else if(itmp==EINVAL)
  {
    printf("[%1d] The implementation has detected that the value",id);
    printf("  specified by thread does not refer to a joinable thread.\n");
  }
  else if(itmp==ESRCH)
  {
    printf("[%1d] No thread could be found corresponding to that",id);
    printf("  specified by the given thread  ID.");
  }

  /* terminate calling thread */
  pthread_exit(NULL);

  return(NULL);
}


/* it effects all threads, should NOT be called from particular thread ! */
#define SHUTDOWN_BUILD \
  if(ebp->nthreads != 0) \
  { \
    int itmp = 0; \
    printf("cancel building threads\n"); \
    for(id=0; id<ebp->nthreads; id++) \
    { \
      if(ebp->idth[id] != 0) \
      { \
        pthread_t build_thread = ebp->idth[id]; \
        ebp->idth[id] = 0; \
        pthread_cancel(build_thread); \
        itmp = pthread_join(ebp->idth[id], &status); \
        printf("status is 0x%08x, itmp=%d\n",status,itmp); \
        if(itmp==EINVAL) \
        { \
          printf("ERROR in pthread_join(): The implementation has detected that the \n"); \
          printf("   value specified by thread does not refer to a joinable thread.\n"); \
        } \
        else if(itmp==ESRCH) \
        { \
          printf("ERROR in pthread_join(): No thread could be found corresponding to \n"); \
          printf("   that specified by the given thread ID.\n"); \
        } \
        else if(itmp==EDEADLK) \
        { \
          printf("ERROR in pthread_join(): A recursive deadlock was detected, the value \n"); \
          printf("   of thread specifies the calling thread.\n"); \
        } \
        if(status && (!itmp)) free(status); \
      } \
    } \
    ebp->nthreads = 0; \
  } \
  debcloselinks()


/***************************/
/* following is for ending */
/***************************/
int
polling_routine()
{
  objClass object = localobject;

  EBp ebp = (EBp) object->privated;
  void *status;
  int id;
  int *ptr;

  if(ebp->ended == 1)
  {
printf("polling_routine ================!!!!!!!!!!!!!!!!!!=====\n");
fflush(stdout);

	SHUTDOWN_BUILD;

	/*
polling_routine ================!!!!!!!!!!!!!!!!!!=====
cancel building threads
Segmentation fault
	*/

    printf("INFO: ended5\n");fflush(stdout);
    ebp->ended = 0;
    printf("INFO: ended4\n");fflush(stdout);
    ebp->ending = 0;
    printf("INFO: ended3\n");fflush(stdout);
    printf("INFO: ended31 (0x%08x)\n",object);fflush(stdout);
    printf("INFO: ended32 (0x%08x)\n",object->state);fflush(stdout);
	/*sergey*/
    free(object->state); //sergey: was 'cfree'
    printf("INFO: ended2 (0x%08x)\n",object->state);fflush(stdout);
    object->state = calloc(12,1);
    printf("INFO: ended1\n");fflush(stdout);
    tcpState = DA_DOWNLOADED;
    codaUpdateStatus("downloaded");
    printf("INFO: ended\n");fflush(stdout);
  }

  return(CODA_OK);
}

void
eb_ended_loop()
{
  printf("eb_ended_loop started\n");fflush(stdout);
  printf("eb_ended_loop started\n");fflush(stdout);
  printf("eb_ended_loop started\n");fflush(stdout);
  printf("eb_ended_loop started\n");fflush(stdout);
  printf("eb_ended_loop started\n");fflush(stdout);

  while(1)
  {
    if(ended_loop_exit)
    {
      ended_loop_exit = 0;
      return;
    }

    polling_routine();

    /* wait 1 sec */
    sleep(1);
  }

  pthread_exit(NULL);

  return;
}

int
deb_constructor()
{
  int i, j;
  char tmp[400];
  EBp ebp, *ebh;
  pthread_mutexattr_t mattr;


  printf("\n\ndeb_constructor reached\n");fflush(stdout);
  /*return(0);*/

  /*
  localobject = object;
  */
  printf("\n\nlocalobject = 0x%08x\n",localobject);fflush(stdout);

  /* allocate and clean up structure 'eb_priv' */
  /* will be accessed by 'localobject->privated' */
  ebp = localobject->privated = (void *) calloc(sizeof(eb_priv),1);
  bzero((char *)ebp, sizeof(eb_priv));

  /* allocate and activate pointer to structure 'eb_priv' */
  ebh = (EBp *) calloc(sizeof(EBp),1);
  *ebh = ebp;

  /* building threads did not started yet */
  ebp->nthreads = 0;
  for(i=0; i<NTHREADMAX; i++) ebp->idth[i] = 0;

  roc_queue_ix = 0; /* the number of ROCs; incremented in LINK_support.c */

  /* initialize mutex etc */
  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
  /*pthread_mutex_init (&ebp->active_mutex, &mattr);*/ /* obsolete ? */
  /*pthread_cond_init (&ebp->active_cond, NULL);*/ /* obsolete ? */
  pthread_mutex_init (&ebp->data_mutex, &mattr);
  pthread_cond_init (&ebp->data_cond, NULL);

  /* start ending thread; it will check in loop if we are ended; if so,
     it will shutdown building thread etc */

  {
    int res;
    pthread_t thread1;
    pthread_attr_t detached_attr;

    pthread_attr_init(&detached_attr);
    pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(&detached_attr, PTHREAD_SCOPE_SYSTEM);

    res = pthread_create( &thread1, &detached_attr,
		   (void *(*)(void *)) eb_ended_loop, (void *) NULL);

    printf("pthread_create returned %d\n",res);fflush(stdout);
    perror("pthread_create");
  }

  tcpState = DA_BOOTED;
  if(codaUpdateStatus("booted") != CODA_OK) return(CODA_ERROR);

  tcpServer(localobject->name, mysql_host); /*start server to process non-coda commands sent by tcpClient*/

  return(CODA_OK);
}


int
deb_destructor()
{
  EBp ebp = (void *) localobject->privated;
  void *status;
  int id;

  /*ebp->active = 0;*/ /* obsolete ? */

  SHUTDOWN_BUILD;
  ended_loop_exit = 1;

  return(CODA_OK);
}



int
codaInit(char *confname)
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
codaDownload(char *confname)
{
  objClass object = localobject;

  EBp  ebp = (void *) object->privated;
  int  id, ix;
  char tmp[1000], tmp2[1000], tmpp[1000];
  int  listArgc;
  char listArgv[LISTARGV1][LISTARGV2];
  MYSQL *dbsock;

  UDP_start();

  tcpState = DA_DOWNLOADING;
  if(codaUpdateStatus("downloading") != CODA_OK) return(CODA_ERROR);


  ebp->force_end = 0;
  if(ebp->nthreads != 0)
  {
    printf("WARN: Can't download while %d build threads are active, END first.\n",
             ebp->nthreads);fflush(stdout);
    return(CODA_ERROR);
  }

  for(id=0; id<nthreads; id++)
  {
    if(ebp->out_id[id])
    {
      printf("INFO: Unloading event encode module %x\n",ebp->out_id[id]);
      if(dlclose ((void *) ebp->out_id[id]) != 0)
      {
        printf("download: failed to unload module to encode >%s<\n",
          ebp->out_name);fflush(stdout);
        return(CODA_ERROR);
      }
    }
  }


  strcpy(configname,confname); /* Sergey: save CODA configuration name */
  printf("INFO: Downloading configuration '%s'\n",configname);fflush(stdout);
  strcpy(ebp->out_name,"CODA");


  /* get run config file name from DB */
  getConfFile(configname, confFile, 255);

  /* connect to database */
  dbsock = dbConnect(mysql_host, expid);
  if(dbsock==NULL)
  {
    printf("cannot connect to the database 1 - exit\n");
    exit(0);
  }
  printf("312: dbsock=0x%08x\n",dbsock);

  /* get the list of readout-lists from the database */
  sprintf(tmpp,"SELECT code FROM %s WHERE name='%s'",configname,object->name);
  if(dbGetStr(dbsock, tmpp, tmp)==CODA_ERROR) return(CODA_ERROR);
  printf("code >%s< selected\n",tmp);

  /* disconnect from database */
  dbDisconnect(dbsock);



  /*
   * Decode configuration string...
   */
  listArgc = 0;
  if(!((strcmp (tmp, "{}") == 0)||(strcmp (tmp, "") == 0)))
  {
    if(listSplit1(tmp, 1, &listArgc, listArgv)) return(CODA_ERROR);
    for(ix=0; ix<listArgc; ix++) printf("nrols [%1d] >%s<\n",ix,listArgv[ix]);
  }
  else
  {
    printf("download: do not split list >%s<\n",tmp);
  }


/* do we still need it ??? */

#ifdef SunOS
  printf("thread concurrency level is %d\n",thr_getconcurrency());
  thr_setconcurrency(100);
  printf("thread concurrency level set to %d\n",thr_getconcurrency());
#endif



  tcpState = DA_DOWNLOADED;
  if(codaUpdateStatus("downloaded") != CODA_OK) return(CODA_ERROR);


/*sergey: just test
tmpUpdateStatistics();
*/

  return(CODA_OK);
}







int
debopenlinks()
{
  objClass object = localobject;

  EBp ebp = (void *) localobject->privated;
  DATA_LINK links[MAX_ROCS];
  int nrocs, rocid, i, len, ix, numRows;
  MYSQL *dbsock;
  MYSQL_RES *result;
  MYSQL_ROW row;
  char tmp[2000], tmpp[1000], tohost[100];

printf("=o=============================================\n");fflush(stdout);
printf("=o=============================================\n");fflush(stdout);
printf("=o=============================================\n");fflush(stdout);
printf("debopenlinks reached\n");fflush(stdout);


  /* cleanup everything from previous configuration(s) */
  for(ix=0; ix<MAX_ROCS; ix++)
  {
	ebp->roc_id[ix] = -1;
    ebp->roc_nb[ix] = -1;
	ebp->links[ix] = NULL;
    for(i=0; i<QSIZE; i++)
	{
      if(bufpool[ix][i] != NULL) free(bufpool[ix][i]);
      bufpool[ix][i] = NULL;
    }
  }
  roc_queue_ix = 0;


  /* select configuration */
  dbsock = dbConnect(mysql_host, expid);
  if(dbsock==NULL)
  {
    printf("cannot connect to the database 2 - exit\n");
    exit(0);
  }

  sprintf(tmp,"SELECT name,inputs,outputs,next FROM %s WHERE name='%s'",
    configname,object->name);
  if(mysql_query(dbsock, tmp) != 0)
  {
    printf("ERROR: cannot select\n");
    return(CODA_ERROR);
  }
  else
  {
    printf("selected\n");
  }

  /* gets results from selected configuration */
  if( !(result = mysql_store_result(dbsock)) )
  {
    printf("ERROR in mysql_store_result()\n");
    return(CODA_ERROR);
  }
  else
  {
    numRows = mysql_num_rows(result);
    printf("nrow=%d\n",numRows);
    if(numRows != 1)
    {
      printf("ERROR: numRows=%d, must be 1\n",numRows);
	}
    else
    {
      if((row = mysql_fetch_row(result)))
	  {
        /* extract fields from the Event Builder line in config table, for example:
	         name                  inputs                    outputs       next 
	        >EB5< >croctest1:croctest1 croctest2:croctest2<     ><           ><   */

        printf("name>%s< inputs>%s< outputs>%s< next>%s<\n",
          row[0],row[1],row[2],row[3]);

        strcpy(tmp,row[1]);
        printf("tmp >%s<\n",tmp);

        linkArgc = 0;
        if(!((strcmp(tmp, "") == 0)||(strlen(tmp) < 3)))
        {
          if(listSplit1(tmp, 0, &linkArgc, linkArgv)) return(CODA_ERROR);
          for(ix=0; ix<linkArgc; ix++)
		  {
            printf("input1 [%1d] >%s<\n",ix,linkArgv[ix]);
		  }
          /* replace ':' by the end of string */
          for(ix=0; ix<linkArgc; ix++)
		  {
            len = strlen(linkArgv[ix]);
            for(i=0; i<len; i++)
            {
              if(linkArgv[ix][i] == ':')
              {
                linkArgv[ix][i] = '\0';
                break;
              }
            }
		  }
          for(ix=0; ix<linkArgc; ix++)
		  {
            printf("input2 [%1d] >%s<\n",ix,linkArgv[ix]);
		  }
	    }
      }
      else
      {
        printf("ERROR: do not split inputs >%s<\n",tmp);
      }

    }

    mysql_free_result(result);
  }


  /* for every ROC from selected configuration, obtain rocid and init/allocate appropriate arrays */
  nrocs = 0;
  for(ix=0; ix<linkArgc; ix++)
  {    
    /* get ROC id from process table */
	sprintf(tmp,"SELECT id FROM process WHERE name ='%s'",linkArgv[ix]);
    if(dbGetInt(dbsock, tmp, &rocid)==CODA_ERROR) return(CODA_ERROR);
    printf("rocid=%d\n",rocid);

	ebp->roc_id[nrocs] = rocid; /* our rocid: DC1 is 1, CC1 is 12 etc */
	ebp->roc_nb[rocid] = nrocs; /* roc numbers: 0,1,2,... */

    /* allocate 'bufpool' for this ROC */
    printf("creating pool of buffers for roc=%d (rocid=%d)\n",ix,rocid); fflush(stdout);
    for(i=0; i<QSIZE; i++)
    {
      if(bufpool[ix][i] != NULL) free(bufpool[ix][i]);

      bufpool[ix][i] = (unsigned int *) malloc(TOTAL_RECEIVE_BUF_SIZE+128);
      if(bufpool[ix][i] == NULL)
      {
        printf("ERROR: cannot allocate buffer - exit.\n");
        fflush(stdout);
        exit(0);
      }
      else
	  {
        printf("[roc %2d][buf %2d] %d bytes has been allocated\n",rocid,i,TOTAL_RECEIVE_BUF_SIZE+128);
	  }
    }

    roc_queues[ix] = cb_init(ix, linkArgv[ix], "EventBuilder");
    /*ebp->roc_stream[ix] = &roc_queues[ix];*/

	nrocs ++;
  }
  ebp->nrocs = nrocs;


  /* for every ROC from selected configuration, create link to EB */
  for(ix=0; ix<linkArgc; ix++)
  {
    /* get the host name where it suppose to send data */
	sprintf(tmp,"SELECT outputs FROM %s WHERE name ='%s'",configname,linkArgv[ix]);
    if(dbGetStr(dbsock, tmp, tmpp)==CODA_ERROR) return(CODA_ERROR);
    printf("tmpp>%s<\n",tmpp);
    len = strlen(tmpp);
    for(i=0; i<len; i++)
    {
      if(tmpp[i]==':')
      {
        strncpy(tohost,&tmpp[i+1],(len-i)); /*copy starting from the symbol right after ':' */
        tohost[len-i] = '\0';
        break;
      }
    }
    printf("tohost>%s<\n",tohost);

    /* create link */
	printf("debOpenLink: >%s< >%s< >%s< 0x%08x\n",linkArgv[ix],object->name,tohost, dbsock);
    ebp->links[ix] = debOpenLink(linkArgv[ix], object->name, tohost, dbsock);
    printf(">>>>> open link from >%s< link=0x%08x\n",linkArgv[ix],ebp->links[ix]);
  }


  dbDisconnect(dbsock);





printf("debopenlinks done\n");fflush(stdout);
printf("=o=============================================\n");fflush(stdout);
printf("=o=============================================\n");fflush(stdout);
printf("=o=============================================\n");fflush(stdout);

  return(CODA_OK);
}



int
debcloselinks()
{
  int ix;
  MYSQL *dbsock;
  EBp ebp = (void *) localobject->privated;

printf("=c=============================================\n");fflush(stdout);
printf("=c=============================================\n");fflush(stdout);
printf("=c=============================================\n");fflush(stdout);

  dbsock = dbConnect(mysql_host, expid);
  if(dbsock==NULL)
  {
    printf("cannot connect to the database 3 - exit\n");
    exit(0);
  }

  /* send force close command to all rocs */
  for(ix=0; ix<linkArgc; ix++)
  {
	printf("=c====\n");fflush(stdout);
    printf(">>>>> force close link from >%s< link=0x%08x\n",linkArgv[ix],ebp->links[ix]);
    debForceCloseLink(ebp->links[ix], dbsock);
  }

  /* wait for links to be closed */
  for(ix=0; ix<linkArgc; ix++)
  {
	printf("=c====\n");fflush(stdout);
    printf(">>>>> check close link from >%s< link=0x%08x\n",linkArgv[ix],ebp->links[ix]);
    debCloseLink(ebp->links[ix], dbsock);
  }

  linkArgc = 0;

  dbDisconnect(dbsock);

printf("=cc============================================\n");fflush(stdout);
printf("=cc============================================\n");fflush(stdout);
printf("=cc============================================\n");fflush(stdout);

  return(CODA_OK);
}










int
codaPrestart()
{
  objClass object = localobject;

  EBp ebp = (void *) object->privated;
  int i, j, ix, id, async, numRows;
  int waitforET = 2*(ET_MON_SEC + 1);
static char temp[100];
static ebArg args[NTHREADMAX];
  void *status;
  char tmp[1000], tmpp[1000];
  MYSQL *dbsocket;
  MYSQL_RES *result;
  MYSQL_ROW row;
  WORD128 roc_mask_local;
  int roc_id_local;

#ifdef SunOS
  printf("thread concurrency level is %d\n",thr_getconcurrency());
  thr_setconcurrency(30);
#endif

  debcloselinks();
  sleep(1);

  /* cleanup another ROCs mask; will be filled by 'handle_link' calles
  started by 'debopenlinks' and compared with 'ebp->roc_mask' in the begining
  of building thread */

  Clear128(&roc_linked);

  sleep(1);

  debopenlinks();


  printf("INFO: Prestarting (C)\n");fflush(stdout);


  /*******************/
  /* setup ET system */

  /* If we need to initialize, reinitialize */
  if((et_init == 0) || (et_reinit == 1))
  {
    if(eb_et_initialize() != CODA_OK)
    {
      printf("ERROR: deb prestart: cannot initialize ET system\n");
      return(CODA_ERROR);
    }
  }

  /* If this is a Linux system check server */
  if((!et_alive(et_sys)) && (et_locality == ET_LOCAL_NOSHARE))
  {	
    if(eb_et_initialize() != CODA_OK)
    {
      printf("ERROR: deb prestart: cannot initialize ET system\n");
      return(CODA_ERROR);
    }
  }
  else if((!et_alive(et_sys)))
  {
    if(waitforET < 5) waitforET = 5;
    sleep(waitforET);
    if(!et_alive(et_sys))
    {
      printf("ERROR: deb prestart: ET system is not responding\n");
      return(CODA_ERROR);
    }
  }
  else
  {
    printf("INFO: deb prestart: ET is alive - EB attached\n");
  }


  /* ERROR: MUST BE SEPARATE FOR EVERY BUILDING THREAD !!! allocate memory for HPS_HACK */
  id = 0;
  MAXBUF = (et_eventsize/4) + 128;
  if(hpsbuf != NULL) free(hpsbuf);
  if((hpsbuf = (int *) calloc(MAXBUF, sizeof(int))) == NULL)
  {
    printf("[%1d] deb ET init: cannot allocate buffer for HPS_HACK - exit\n",id);
    exit(0);
  }
  else
  {
    printf("[%1d] deb ET init: allocated buffer for HPS_HACK, size=%d words\n",id,MAXBUF);
  }





  /* init events ordering queues */
  id_in_index = 0;
  id_out_index = 0;
  for(id=0; id<NIDMAX; id++)
  {
    id_in[id] = id;
    id_out[id] = id;
  }
  pthread_mutex_init(&id_out_lock, NULL);
  pthread_cond_init(&id_out_empty, NULL);

  /* some cleanups */
  ebp->force_end = 0;
  ebp->ended = 0;
  ebp->end_event_done = 0;

  printf("11\n");fflush(stdout);

  /* Initialize all queues */
  for(ix=0; ix<ebp->nrocs; ix++) roc_queues[ix]->deleting = 0;

  printf("12\n");fflush(stdout);

  /* connect to database */
  dbsocket = dbConnect(mysql_host, expid);
  if(dbsocket==NULL)
  {
    printf("cannot connect to the database 4 - exit\n");
    exit(0);
  }

  /**********************/
  /* Get the run number */
  /**********************/
  sprintf(tmpp,"SELECT runNumber FROM sessions WHERE name='%s'",session);
  if(dbGetInt(dbsocket, tmpp, &(object->runNumber))==CODA_ERROR) return(CODA_ERROR);

  /******************************************************/
  /* get the run type number and save it somewhere safe */
  /******************************************************/
  sprintf(tmpp,"SELECT id FROM runTypes WHERE name='%s'",configname);
  if(dbGetInt(dbsocket, tmpp, &(object->runType))==CODA_ERROR) return(CODA_ERROR);

  printf("INFO: prestarting, run=%d, type=%d\n",object->runNumber,object->runType);

  object->nevents = 0;
  object->nlongs = 0;


  /**************************************************/
  /* check which rocs are active and setup roc mask */
  /**************************************************/


  /****************************/
  /* get 'inuse' ROCs from DB */
  /****************************/

  /* get all rows from config table */
  sprintf(tmp,"SELECT name,outputs,inuse FROM %s",configname);
  if(mysql_query(dbsocket, tmp) != 0)
  {
    printf("ERROR: cannot select\n");
    return(CODA_ERROR);
  }
  else
  {
    printf("selected\n");
  }


  /* gets results from previous query */
  if( !(result = mysql_store_result(dbsocket)) )
  {
    printf("ERROR in mysql_store_result()\n");
    return(CODA_ERROR);
  }
  else
  {
#if 0
    FILE *fd;
    char *clonparms = getenv("CLON_PARMS");
    char foutname[256];
#endif

    numRows = mysql_num_rows(result);
    printf("nrow=%d\n",numRows);

    Clear128(&roc_mask_local);

#if 0
    /* open 'current_daq_config.txt' for writing */
    sprintf(foutname, "%s/trigger/current_daq_config.txt",clonparms);
    if((fd=fopen(foutname,"w")) == NULL)
    {
      printf("\nWARN: Can't open file >%s< to upload current daq configuration\n",foutname);
    }
    else
    {
      printf("Writing file >%s<\n",foutname);
    }
#endif

    for(ix=0; ix<numRows; ix++)
    {
      row = mysql_fetch_row(result);
      printf("[%1d] received from DB >%s< >%s< >%s<\n",ix,row[0],row[1],row[2]);

#if 0
      if(fd != NULL) /* write to 'current_daq_config.txt' */
      {
        fprintf(fd,"%s %s",row[0],row[2]);
      }
#endif

      if( strncmp(row[2],"no",2) != 0 ) /* 'inuse' != 'no' */
      {
        roc_id_local = atoi(row[2]);
        if((roc_id_local>=0) && (roc_id_local<MAX_ROCS))
        {
          SetBit128(&roc_mask_local,roc_id_local);
        }
      }
    }

#if 0
    if(fd!=NULL) fclose(fd); /* close 'current_daq_config.txt' */
#endif

    mysql_free_result(result);
  }


  Copy128(&roc_mask_local, &ebp->roc_mask);
  Print128(&roc_mask_local);

  /**********************************************/
  /* set roc mask in mysql so TS can read it out */
  /**********************************************/
  String128((WORD128 *)&ebp->roc_mask, temp, 99);
printf("mysql request temp >%s<\n",temp);fflush(stdout);
  sprintf(tmpp,"SELECT name,value FROM %s_option WHERE name='rocMask'",configname);
printf("mysql request >%s< (temp>%s<)\n",tmpp,temp);fflush(stdout);
  if(mysql_query(dbsocket, tmpp) != 0)
  {
    printf("ERROR in mysql_query\n");
  }
  else
  {
    if( !(result = mysql_store_result(dbsocket)) )
    {
      printf("ERROR in mysqlStoreResult()\n");
      return(CODA_ERROR);
    }
    else
    {
      numRows = mysql_num_rows(result);
      printf("nrow=%d\n",numRows);
      if(numRows == 0)
      {
        sprintf(tmpp,"INSERT INTO %s_option (name,value) VALUES ('rocMask','%s')",configname,temp);
        printf(">%s<\n",tmpp);
        if(mysql_query(dbsocket,tmpp) != 0)
        {
          printf("ERROR in INSERT\n");
        }
      }
      else if(numRows == 1)
      {
        sprintf(tmpp,"UPDATE %s_option SET value='%s' WHERE name='rocMask'\n",configname,temp);
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


  /* disconnect from database */
  dbDisconnect(dbsocket);









  /**************************/
  /* start building threads */
  /**************************/

  ebp->nthreads = nthreads;
  for(id=0; id<ebp->nthreads; id++)
  {
    pthread_attr_t detached_attr;

    pthread_attr_init(&detached_attr);
    pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(&detached_attr, PTHREAD_SCOPE_SYSTEM);

    printf("Start building thread [%1d]\n",id);

    args[id] = (ebArg) calloc(sizeof(EBARGS),1);
    args[id]->object = object;
    args[id]->id = id;
    pthread_create(&ebp->idth[id], /*Sergey: try to detach NULL*/&detached_attr,
                   (void *(*)(void *)) handle_build, (void *) args[id]);
  }

  /*ERROR: sergey: we are telling rcServer that EB is ready, so rcServer can go with rocs prestarting;
  'handle_build' will wait for all connections from all rocs; it may happens that some connection(s)
  will not be established, but all rocs will report to rcServer, in that case 'Go' button will
  appeares while EB still complaining 'waiting for the following ROC IDs'; need to do something
  about that ... */
  tcpState = DA_PAUSED;
  if(codaUpdateStatus("paused") != CODA_OK)
  {
    printf("--> return ERROR\n"); fflush(stdout);
    return(CODA_ERROR); 
  }

  ievent_old = 0;

  printf("--> codaPrestart returns OK\n"); fflush(stdout);

  return(CODA_OK);
}

int
codaGo()
{
  objClass object = localobject;

  EBp ebp = (void *) object->privated;

  printf("INFO: Activating (C)\n");
  ebp->ended = 0;
  ebp->end_event_done = 0;

  tcpState = DA_ACTIVE;
  if(codaUpdateStatus("active") != CODA_OK)

  return(CODA_OK);
}  


/* called ad 'End' transition; building thread must receive 'end'
command INDEPENDENTLY (through event type EV_END or error condition)
and polling_routine will see it and end, so here we just waiting
for 'downloaded'; if it is not happeded during ... sec, we will
force end (TO DO) */





/* when end clicked, following printed (GOES SECOND, AFTER ROC):

CODAtcpServer: start work thread
CODAtcpServer: befor: socket=6 address>129.57.86.64< port=59685
CODAtcpServer: wait: coda request >go< in progress
CODAtcpServerWorkTask entry
codaExecute reached, message >end<, len=3
codaExecute: 'end' transition
codaEnd 1
codaEnd 2
codaEnd 3
codaEnd 4
codaEnd 5
>>>>>>>>>> name >EBDAQ4< state >active<
codaEnd 6
deb_end reached
deb_end: state is active
codaEnd 7
===============================================
===============================================
===============================================
===============================================
===============================================
codaEnd 8
INFO: Ending (C)
codaEnd 9
deb_end: 1 threads still alive - waiting 0 times ..
<<< time2=      5 microsec (nevents2put=64) >>>
CODA_decode_spec: len=12 type=148
[0] WE ARE NOT ENDING BUT GOT END EVENT FROM ROC 0
[0] force_end 1 end_event_done 0
128> oooooooooooooooooooooooooooooooo oooooooooo|ooooooooooooooooooooo oooooooooooooooooooooooooooooooo oooooooooooooooooooooooooooooooo 
[0] NEED TO TEST AND PROBABLY REDESIGN THAT PART !!!
[0] EVENT_ENCODE_SPEC 2 ..
[0] .. done
[0] handle_build_cleanup
[0] flush ET system
[0] detached from ET
[0] remove mutex locks and shutdown fifos

[0] flushing input streams
get_cb_count: count = 0
[0] count for roc[00] = 0
get_cb_count: count = 0

[0] flushing input streams
get_cb_count: count = 0
[0] count for roc[01] = 0
get_cb_count: count = 0

[0] flushing input streams
get_cb_count: count = 0
[0] count for roc[02] = 0
get_cb_count: count = 0

[0] flushing input streams
get_cb_count: count = 0
[0] count for roc[03] = 0
get_cb_count: count = 0

[0] flushing input streams
get_cb_count: count = 0
[0] count for roc[04] = 0
get_cb_count: count = 0

[0] flushing input streams
get_cb_count: count = 0

................

[0] flushing input streams
get_cb_count: count = 0
[0] count for roc[125] = 0
get_cb_count: count = 0

[0] flushing input streams
get_cb_count: count = 0
[0] count for roc[126] = 0
get_cb_count: count = 0
[0] ============= Build threads cleaned
[0] ============= Build threads cleaned
[0] ============= Build threads cleaned
[0] ============= Build threads cleaned
[0] ============= Build threads cleaned
[0] build thread exiting: 1 1
[0] The implementation has detected that the value  specified by thread does not refer to a joinable thread.
polling_routine ================!!!!!!!!!!!!!!!!!!=====
cancel building threads
=c=============================================
=c=============================================
=c=============================================
=c====
>>>>> close link from >ROC85< link=0xfc007560
debCloseLink: theLink=0xfc007560 -> closing
debCloseLink reached, fd=8 sock=9
11: shutdown fd=8
12
debCloseLink: socket fd=8 sock=9 connection closed
903 8
904 8
905 8
906
[ 8] LINK_sized_read(): closed
[ 8] handle_link(): LINK_sized_read() returns 0
[ 8] handle_link(): put_cb_data calling ...
[ 8] PUT: fifo is being emptied !
[ 8] handle_link(): put_cb_data called
[ 8] handle_link(): thread exit
[ 8] 907
debCloseLink: link is down
debCloseLink: free memory
debCloseLink: done.
=cc============================================
=cc============================================
=cc============================================
INFO: ended5
INFO: ended4
INFO: ended3
INFO: ended31 (0x01427970)
INFO: ended32 (0x0140da40)
INFO: ended2 (0x0140da40)
INFO: ended1
codaUpdateStatus: dbConnecting ..
codaUpdateStatus: dbConnect done
codaUpdateStatus(table 'process'): >UPDATE process SET state='downloaded' WHERE name='EBDAQ4'<
codaUpdateStatus(table 'sessions'): >UPDATE sessions SET log_name='downloaded' WHERE name='clastest1'<
codaUpdateStatus: dbDisconnecting ..
codaUpdateStatus: dbDisconnect done
codaUpdateStatus: updating request ..
UDP_standard_request >sta:EBDAQ4 downloaded<
UDP_standard_request >sta:EBDAQ4 downloaded<
UDP_standard_request >sta:EBDAQ4 downloaded<
UDP_standard_request >sta:EBDAQ4 downloaded<
UDP_standard_request >sta:EBDAQ4 downloaded<
UDP_standard_request >sta:EBDAQ4 downloaded<
UDP_cancel: cancel >sta:EBDAQ4 active<
codaUpdateStatus: updating request done
INFO: ended
CODAtcpServer: wait: coda request >end< in progress
codaEnd 10
codaEnd 11
DEB_END: all threads are exited
codaEnd 10
codaEnd 11
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
codaExecute done
CODAtcpServerWorkTask exit ?
CODAtcpServerWorkTask exit !
*/


int
codaEnd()
{
  objClass object = localobject;

  int i, j, itmp, count;
  MYSQL *dbsocket;
  char tmp[1000], tmpp[1000];
  EBp ebp = (void *) object->privated;

printf("codaEnd 1\n");fflush(stdout);

/*kuku: always downloaded*/
  /* get current state */
printf("codaEnd 2\n");fflush(stdout);
  dbsocket = dbConnect(mysql_host, expid);
  if(dbsocket==NULL)
  {
    printf("cannot connect to the database 5 - exit\n");
    exit(0);
  }
printf("codaEnd 3\n");fflush(stdout);
  sprintf(tmp,"SELECT state FROM process WHERE name='%s'",object->name);
printf("codaEnd 4\n");fflush(stdout);
  if(dbGetStr(dbsocket, tmp, tmpp)==CODA_ERROR) return(CODA_ERROR);
printf("codaEnd 5\n");fflush(stdout);
printf(">>>>>>>>>> name >%s< state >%s<\n",object->name,tmpp);
  dbDisconnect(dbsocket);
printf("codaEnd 6\n");fflush(stdout);

  printf("deb_end reached\n");fflush(stdout);
  printf("deb_end: state is %s\n",tmpp);
printf("codaEnd 7\n");fflush(stdout);
  fflush(stdout);
  printf("===============================================\n");
  printf("===============================================\n");
  printf("===============================================\n");
  printf("===============================================\n");
  printf("===============================================\n");
  fflush(stdout);
  
printf("codaEnd 8\n");fflush(stdout);
  if(strcmp(tmpp,"downloaded") == 0)
  { 
    printf("INFO: already ended, got all end(s) from ROC(s)\n");
printf("codaEnd 81\n");fflush(stdout);

/* ??? we are 'downloaded' already, why doing following ??? */
    tcpState = DA_DOWNLOADED;
    if(codaUpdateStatus("downloaded") != CODA_OK) return(CODA_ERROR);

printf("codaEnd 82\n");fflush(stdout);
  }
  else
  {
    printf("INFO: Ending (C)\n");


	/* SHOULD WAIT .. SEC and INITIATE FORCE END - TO DO */
	/*
    tcpState = DA_ENDING;
    if(codaUpdateStatus("ending") != CODA_OK) return(CODA_ERROR);
    ebp->ending = 1;
	*/

/*kuku: sometimes ending goes after downloaded; is it here ?? */
	/* Sergey: commented out for a while ..
Tcl_Eval(Main_Interp, "dp_after 60000 $this force_end");
	*/

  }
  







  /* wait until all threads are ended */
  count = 0;
printf("codaEnd 9\n");fflush(stdout);
  do
  {
    itmp = 0;
    for(i=0; i<ebp->nthreads; i++)
    {
      if(ebp->idth[i] != 0) itmp ++;
    }
    if(itmp>0)
    {
      printf("deb_end: %d threads still alive - waiting %d times ..\n",
             itmp,count);
      count ++;
      sleep(1);
    }
    else
    {
      printf("DEB_END: all threads are exited\n");
    }
printf("codaEnd 10\n");fflush(stdout);
	
    if(count > 100)
    {
	  /*
      pthread_cond_broadcast(&id_out_empty);
	  */
      printf("deb_end: set 'deleting=1' for all rocs\n");

      /* should call cb_delete() instead of following piece .. */
      for(j=0; j<ebp->nrocs; j++)
      {
          CIRCBUF *f = roc_queues[j];
          f->deleting = 1;
          pthread_cond_broadcast(&f->read_cond);
          pthread_cond_broadcast(&f->write_cond);
      }

    }
printf("codaEnd 11\n");fflush(stdout);

    if(count > 110) /* ????? */
    {
      printf("DEB_END: thread(s) do not respond - do not wait any more\n");
      for(i=0; i<NTHREADMAX; i++) ebp->idth[i] = 0;
      break;
	}
	
  } while(itmp>0);

  ebp->nthreads = 0;
  /*
  for(i=0; i<NTHREADMAX; i++) ebp->idth[i] = NULL;
  */
  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  fflush(stdout);

  return(CODA_OK);
}

int
codaPause()
{
  return(CODA_OK);
}


/* called from 'deb_end' on timeout */
int
debForceEnd()
{
  objClass object = localobject;

  EBp ebp = (void *) object->privated;

  if(ebp->ending)
  {
    printf("Force end, close datalinks ..\n"); fflush(stdout);
    ebp->force_end = 99;
  }

  return(CODA_OK);
}


/* 'Reset' transition ('exit' message) */
int
debShutdownBuild()
{
  objClass object = localobject;

  EBp ebp = (void *) object->privated;
  void *status;
  int id, i;

printf("debShutdownBuild 1\n");

  /* set force end flag */
  ebp->force_end = 1;

  for(i=0; i<ebp->nrocs; i++) cb_delete(i);

  /*SHUTDOWN_BUILD; do not need: cb_delete will force handle_build to exit*/
printf("debShutdownBuild 2\n");

  sleep(3); /* handle_build thread will change status to 'downloaded';
			   wait and change it to 'configured' */

printf("debShutdownBuild 3\n");
  tcpState = DA_CONFIGURED;
  codaUpdateStatus("configured");
printf("debShutdownBuild 4\n");

  return(CODA_OK);
}

int
codaExit()
{
  debShutdownBuild();
  /*
  UDP_reset();
  */
  return(CODA_OK);
}

/*******************************************************/
/*******************************************************/
/*******************************************************/


/*
void
eb_udp_loop()
{
  printf("eb_udp_loop started\n");fflush(stdout);
  printf("eb_udp_loop started\n");fflush(stdout);
  printf("eb_udp_loop started\n");fflush(stdout);
  printf("eb_udp_loop started\n");fflush(stdout);
  printf("eb_udp_loop started\n");fflush(stdout);

  udpsocket = UDP_establish(udphost, udpport);

  while(1)
  {
    if(udp_loop_exit)
    {
      udp_loop_exit = 0;
      UDP_close(udpsocket);
      return;
    }
    UDP_send(udpsocket);

    sleep(1);
  }

  pthread_exit(NULL);

  return;
}
*/


/****************/
/* main program */
/****************/

void
main (int argc, char **argv)
{
printf("1\n");fflush(stdout);
  CODA_Init(argc, argv);
printf("2\n");fflush(stdout);

  deb_constructor();

  /* CODA_Service ("ROC"); */
  CODA_Execute ();
}

#endif
