

/*trying to use big buffers directly*/
#define NEW_READOUT

/*
#include "ttbosio.h"
*/

/* circbuf.h */

/* uncomment following line if do NOT want to send data from ROC to EB */
//#define ROC_DOES_NOT_SEND


/* uncomment following line if do NOT want to send data to Building Thread */
//#define DO_NOT_PUT


/* uncomment following line if do NOT want to actually build */
//#define DO_NOT_BUILD 1







/****************************************************************************/
/* allocate smaller buffers until bigger memory will be avail */

#ifdef Linux
#define NWBOS 524288
#else
#define NWBOS (524288/4) /*(MAX_EVENT_LENGTH/4)*/
#endif


/* the number of buffers in EB, normally 8 */
#ifdef Linux_x86_64_RHEL9
#define QSIZE 32
#else
#define QSIZE 8
#endif


#ifdef Linux

#define MAX_EVENT_LENGTH (NWBOS*4)
#define MAX_EVENT_POOL   400

/*buffer size in bytes*/
#ifdef Linux_x86_64_RHEL9
#define SEND_BUF_SIZE  (32 * 1024 * 1024)
#else
#define SEND_BUF_SIZE  (8 * 1024 * 1024)
#endif

#define TOTAL_RECEIVE_BUF_SIZE  SEND_BUF_SIZE

#else

/* ROL1 buffers */
#define MAX_EVENT_LENGTH (NWBOS*4) /* event buffer length in bytes */
#define MAX_EVENT_POOL   /*400*/200       /* the number of event buffers */
#define SEND_BUF_SIZE  (4 * 1024 * 1024)
#define TOTAL_RECEIVE_BUF_SIZE  SEND_BUF_SIZE

#endif




/* MAX_ROCS must accomodate biggest roc_id, not the number of rocs !!! biggest roc_id must be <MAX_ROCS*/
#ifdef USE_128
#define MAX_ROCS 128
#else
#define MAX_ROCS 256
#endif



#define NTHREADMAX 7
#define NFIFOMAX   4000 /* the number of events in ET system */
#define NIDMAX     7 /*(NFIFOMAX+NTHREADMAX)*/
#define NCHUNKMAX  110





/* big buffer defines */
#define BBIWORDS   0  /* index of buffer length in words */
#define BBIBUFNUM  1  /* index of buffer number */
#define BBIHEAD    2  /* the number of words in big buffer header = 8 */
#define BBIEVENTS  3  /* index of the number of events in buffer */
#define BBIROCID   4  /* index of ROC id */
#define BBVERSION  5  /* version(4), is last block (1), payload bank type (0), has first event (0) */
#define BBIEND     6  /* index of 'end' condition */ 
#define BBIFD      7  /* index of socket descriptor, also used to store 'magic'  */
#define BBHEAD     8  /* the number of words in big buffer header = 8 */
#define BBHEAD_BYTES (BBHEAD*4) /* the number of bytes in big buffer header */





/* minimum number of network buffers required to proceed; if it fells below
warning message will be printed; we may consider pospond execution until
system will recycle buffers (it takes 1 minute) */
#define MINNETBUFS 100



typedef struct circbuf
{
  /* general info */
  int roc;                     /* the ordered number of the ROC (from 0) */
  int rocid;                   /* ROC id (from CODA database) */
  char name[100];              /* name given by user (just info) */
  char parent[100];            /* parent name (just info) */

  /* buffers */
  int write;  /* changed by 'put' only */
  int read;   /* changed by 'get' only */
  void *data[QSIZE];           /* circular buffer of pointers */

  /* locks and conditions */
  pthread_mutex_t read_lock;   /* lock the structure */
  pthread_cond_t read_cond;    /* full <-> not full condition */

  pthread_mutex_t write_lock;  /* lock the structure */
  pthread_cond_t write_cond;   /* empty <-> notempty condition */

  int deleting;                /* are we in a delete somewhere? */

  /* events info */
  int nevents[QSIZE];          /* the number of events left in the buffer */
  unsigned int *evptr1[QSIZE]; /* pointer to first valid event in the buffer */

  /* attachments info */
  int nattach[QSIZE];          /* the number of attachments to the buffer */
  int nbuf[NTHREADMAX];        /* the number of buffers kept by thread */
  int buf1[NTHREADMAX];        /* first buffer kept by thread */

} CIRCBUF;


/* function prototypes for circbuf.c */

#ifdef  __cplusplus
extern "C" {
#endif

CIRCBUF *cb_init(int roc, char *name, char *parent);
void     cb_delete(int roc);
int      put_cb_data(int fd, CIRCBUF **cbh, void *data);
int      get_cb_data(CIRCBUF **cbh, int id, int chunk, unsigned int *evptr[NCHUNKMAX], int *buflen, int *rocid);
char    *get_cb_name(CIRCBUF **cbh);
int      get_cb_count(CIRCBUF **cbp);
int      cb_events_get(CIRCBUF *cba[MAX_ROCS], int id, int nrocs, int chunk, unsigned int *evptrr[MAX_ROCS][NCHUNKMAX], int *nphys);

#ifdef  __cplusplus
}
#endif
