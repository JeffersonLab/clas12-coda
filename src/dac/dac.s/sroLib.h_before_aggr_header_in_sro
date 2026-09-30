
/* sroLib.h */

#define ABS(x)   ((x) < 0 ? -(x) : (x))

#define TIMERL_VAR \
  static hrtime_t startTim, stopTim, dTim; \
  static int nTim; \
  static hrtime_t  TTT, Tim, rmsTim, minTim=10000000, maxTim, normTim=1

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
      TTT = Tim/nTim/normTim; \
      if(TTT>500LL) {printf("timer[%d]: %7llu microsec (min=%7llu max=%7llu)\n", id_macros, TTT,minTim/normTim,maxTim/normTim);fflush(stdout);} \
      nTim = Tim = 0; \
    } \
  } \
  else \
  { \
    /*logMsg("bad:  %d %ud %ud -> %d\n",nTim,startTim,stopTim,Tim,5,6);*/ \
  } \
}



#define NUM_SEND_BUFS 16  //used by coda_sro.c only, at least now ...
//#define SEND_BUF_SIZE (2 * 1024 * 1024) /*bytes*/ //defined in circbuf.h

#define MAXBUF  (SEND_BUF_SIZE/4) /*MAXBUF =  SEND_BUF_SIZE in words <- assume SEND_BUF_SIZE in bytes !!!*/
#define NFRAMES 10 /*the number of frames in big buffer between input and fb threads ('send' condition)*/

#define MAXSLOT 21
#define MAXCHAN 16
#define MAXDCRBCHAN 96

#define TICKS_PER_BIN  8                         /*the number of ticks in one timing bin; 8 means that one bin will be 8ticks=32ns*/
#define NARRAY         (65536/4/TICKS_PER_BIN)   /* range of one frame in bins */


/*evio*/
#define MAXEVIOBUF (1024*1024) /*words*/
/*evio*/


#define MIN_HITS_IN_BIN         5
#define MIN_DCRBS_IN_BIN        20

#define WINDOW_BEFOR_IN_BINS    20
#define WINDOW_AFTER_IN_BINS    20

#pragma pack(push, 4)
// CODA SRO format (input data format)
typedef struct
{
  unsigned int total_length; /* inclusive total buffer length (32bit words) */
  unsigned int record_counter; /* just incremented for every sent record - does NOT show lost frames !!! */
  unsigned int header_length; /* 8 in our case */
  unsigned int event_count; /* 1 in our case */
  unsigned int roc_id;
  unsigned int evio_version; /* 0x1000|0x200|4 *//*evio version (10th bit indicates last block ?)*/
  unsigned int frame_counter; /* does not come from VTP - filled by input thread using info from payload; shows lost frames */
  unsigned int magic; /* c0da0100 */
} EVIO_Record_Header_t;
#define RECORD_HEADER_LENGTH 8 /*record header length in 32bit words */



/*aggregated data header
 SS: [7]-err, [6:0]-total streams
*/
typedef struct
{
  uint32_t total_length;  /* exclusive length of the entire aggregated data */
  uint32_t w60;           /* [31:16]-0xff60, [15:8]-0x10, [7:0]-SS */
  uint32_t header_length; /* exclusive length of aggregative header */
  uint32_t w31;           /* [31:16]-0xff31, [15:8]-0x20, [7:0]-SS */
  uint32_t w32;           /* [31:24]-0x32, [23:16]-0x1, [15:0]-TSS length */
  uint32_t frame_number;  /* frame number */
  uint32_t timestamp_l;   /* average timestamp, low half */
  uint32_t timestamp_h;   /* average timestamp, high half */
  uint32_t w42;           /* [31:24]-0x42, [23:16]-0x1, [15:0]-AIS length */
  
  /*following words (ROC1, ROC2, ROC3,...) corresponds to input streams
    and have following format: [31:16]-roc_id, [15:8]-SS2, [7:0]-SS1
         SS1 - duplicate from SS coming from stream
         SS2 - [15]-err, [14:8]-reserved (err=1 for missing frames, reserved=low 7 bits of missing ...
  */

} EVIO_Aggregated_Header_t;
#define AGGR_HEADER_LENGTH 9 /* does not count ROC1, ROC2, ROC3,... words */
#define AGGR_HEADER_MAX_LENGTH (AGGR_HEADER_LENGTH+MAXLINKS) /* counting ROC1, ROC2, ROC3, ... */



#pragma pack(pop) /* ? */



#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


typedef int (*IFUNCPTR) ();



/***********************************/
/* structure for sro_input_threads */

typedef struct links *LINKS;
typedef struct links
{
  /* thread id */
  pthread_t thread;

  /* inputs to thread */
  int  ithread; /* assign thread number (from 0) before thread start up */
  char name[MAXSTRLEN];
  char host[MAXSTRLEN];
  int32_t nevents;
  int64_t nlongs;
  int livetime;

  /*input to thread from VTPs */
  int port_in;          /* listening port */
  int socket_in;        /* listening socket (bind) */
  int fd_in;            /* accepted socket (returned by accept()) */

  /*outbut buffer*/
  SROBUF *outbuf;

  int dtime_updated;
  int database_updated;
  int exit;

  /*ERROR: what about non-FADCS ??????????????????????*/
  unsigned int CODASRO_Payload[128*2048]; // Maximum data possible for FADC250 Time/Charge trigger format (65us frame, 128ch)

} LINK_S;


/*******************************/
/* structure for sro_fb_thread */

typedef struct fbs *FBS;
typedef struct fbs
{
  /* thread id */
  pthread_t thread;
  int ithread;      /* assign fb thread number (from 0) before thread start up */

  SROBUF *inbuf[MAXLINKS];
  SROBUF *outbuf[MAXOUTS];

  uint64_t mask_needed;   // MAXLINKS cannot exceed 64 (for now) !!!
  int      mask_num_bits; // the number of active bits in mask_needed = the number of streams we are reading from

  int res;
  int exit;

} FB_S;


/***********************************/
/* structure for sro_output_thread */

typedef struct out *OUT;
typedef struct out
{
  /* thread id */
  pthread_t thread;
  int ithread;      /* assign output thread number (from 0) before thread start up */

  char host[MAXSTRLEN]; /* our host */
  int port_out;         /* listening port */

  int socket_out; /* listening socket (bind) */
  int fd_out;     /* accepted socket (returned by accept()) */

  SROBUF *inbuf;

  int res;
  int exit;

} OUT_S;


/**********************************/
/* structure for sro_proc_threads */

//typedef unsigned short (*qq_t)[MAXSLOT][MAXCHAN][NARRAY];

typedef struct proc *PROC;
typedef struct proc
{  
  /* thread id */
  pthread_t thread;
  int ithread;      /* assign proc thread number (from 0) before thread start up */

  char host_in[MAXSTRLEN]; /* host to connect to */
  int port_in;             /* port to connect to */
  int connected;
  int *bufin;

  int tot_len;             /* exclusive length of the current buffer (words) */
  int index;               /* current buffer index */
  int32_t nsets;           /* the number of sets of frames received (updated on the first received frame from new set) */
  int32_t nevents;         /* the number of events found */
  int64_t nlongs;          /* the number of words in events */
  int livetime;

  /*builder area*/
  int nrocs_active;
  int nstreams_active;
  int roc_id2rocid[MAXROCS]; //index is rocid which can be as high as 127
  int rocid2roc_id[NROCS]; //index is the ron number in configuration (0,1,2,..)

  /*output file*/
  int  output_file;
  char filename[256];
  int output_handle;
  int *bufout;

  int res;
  int exit;

} PROC_S;


typedef struct ETpriv *ETp;
typedef struct ETpriv
{
  objClass object;

  char *session;

  /* params */
  int nevents;
  int event_size;

  int serverPort;
  int udpPort;
  int sendBufSize;
  int recvBufSize;

  char filename[128];
  char subdirname[128];
  char subfilename[128];
  int  output_file;
  int usesubdir;

  /* variables */
  int       nthreads;
  int       nfbs;

  char      name[MAXLINKS][MAXSTRLEN];
  char      host[MAXLINKS][MAXSTRLEN];

  int       exit;

  pthread_t links_id[MAXLINKS];
  LINKS     links[MAXLINKS];

  /* frame builder */
  pthread_t fbs_id[MAXFBS];
  FBS       fbs[MAXFBS];

  pthread_t outs_id[MAXOUTS];
  OUT       outs[MAXOUTS];

  pthread_t procs_id[MAXOUTS];
  PROC      procs[MAXOUTS];

} ET_priv;
static ET_priv ETP;



/************/
/* routines */

int read_socket(int connection, unsigned char *buf, int len, int swap, int ix);
int read_socket_timed(int connection, unsigned char *buf, int len, int swap, long seconds);
int buffercheck1(int *dabufpi, PROC theProc, int print);
int sroGetSet(int *dabufpi, PROC theProc, int print, int *dataindex, int ithread);
int sroPrintSet(int *buf);
int sroEventBuilder(int *bufin, int *bufout, int *evind);
void sroStatPrint(int clean);




/**********************/
/* translation tables */


/*ECAL*/

  /* ecal io (INNNER-OUTER) */
static int adcioecal_full[22][16] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*slot 3*/
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*slot 4*/
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*slot 5*/
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*slot 6*/
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*slot 7*/
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*slot 8*/
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, /*slot 9*/
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /*slot 10*/
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /*slot 13*/
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /*slot 14*/
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /*slot 15*/
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /*slot 16*/
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /*slot 17*/
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, /*slot 18*/
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* ecal layer (U-V-W) */
static int adclayerecal_full[22][16] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*slot 3*/
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*slot 4*/
  1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /*slot 5*/
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /*slot 6*/
  2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, /*slot 7*/
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, /*slot 8*/
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, /*slot 9*/
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*slot 10*/
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*slot 13*/
  1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /*slot 14*/
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /*slot 15*/
  2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, /*slot 16*/
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, /*slot 17*/
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, /*slot 18*/
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

  /*ecal strip numbers*/
static int adcstripecal_full[22][16] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16, /*slot 3*/
 17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32, /*slot 4*/
 33,34,35,36, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12, /*slot 5*/
 13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28, /*slot 6*/
 29,30,31,32,33,34,35,36, 1, 2, 3, 4, 5, 6, 7, 8, /*slot 7*/
  9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24, /*slot 8*/
 25,26,27,28,29,30,31,32,33,34,35,36, 0, 0, 0, 0, /*slot 9*/
  1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16, /*slot 10*/
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32, /*slot 13*/
 33,34,35,36, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12, /*slot 14*/
 13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28, /*slot 15*/
 29,30,31,32,33,34,35,36, 1, 2, 3, 4, 5, 6, 7, 8, /*slot 16*/
  9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24, /*slot 17*/
 25,26,27,28,29,30,31,32,33,34,35,36, 0, 0, 0, 0, /*slot 18*/
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};



/*PCAL*/

static int adclayerpcal_full[22][16] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*slot 3*/
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*slot 4*/
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*slot 5*/
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*slot 6*/
  1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /*slot 7*/
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /*slot 8*/
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /*slot 9*/
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /*slot 10*/
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, /*slot 13*/
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, /*slot 14*/
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, /*slot 15*/
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, /*slot 16*/
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static int adcstrippcal_full[22][16] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16, /*slot 3*/
 17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32, /*slot 4*/
 33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48, /*slot 5*/
 49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64, /*slot 6*/
 65,66,67,68, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12, /*slot 7*/
 13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28, /*slot 8*/
 29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44, /*slot 9*/
 45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60, /*slot 10*/
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 61,62, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14, /*slot 13*/
 15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30, /*slot 14*/
 31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46, /*slot 15*/
 47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62, /*slot 16*/
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};



/*DCRB*/

/*drift chamber layers*/
static int board_layer[96] = {
  2, 4, 6, 1, 3, 5, 2, 4, 6, 1, 3, 5, 2, 4, 6, 1,
  3, 5, 2, 4, 6, 1, 3, 5, 2, 4, 6, 1, 3, 5, 2, 4,
  6, 1, 3, 5, 2, 4, 6, 1, 3, 5, 2, 4, 6, 1, 3, 5,
  2, 4, 6, 1, 3, 5, 2, 4, 6, 1, 3, 5, 2, 4, 6, 1,
  3, 5, 2, 4, 6, 1, 3, 5, 2, 4, 6, 1, 3, 5, 2, 4,
  6, 1, 3, 5, 2, 4, 6, 1, 3, 5, 2, 4, 6, 1, 3, 5
};

/*drift chamber wires*/
static int board_wire[96] = {
  1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3,
  3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 6, 6,
  6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8,
  9, 9, 9, 9, 9, 9,10,10,10,10,10,10,11,11,11,11,
 11,11,12,12,12,12,12,12,13,13,13,13,13,13,14,14,
 14,14,14,14,15,15,15,15,15,15,16,16,16,16,16,16
};


