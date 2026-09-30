
#ifndef _BIGBUF_
#define _BIGBUF_

/* bigbuf.h */

/* max number of buffers allowed */
#define NBIGBUFFERS 32 /* 16 */

#define SEND_BUF_MARGIN  (SEND_BUF_SIZE/4) /*(MAX_EVENT_LENGTH + 128)*/

typedef struct bigbuf
{
  int id; /* buffer id, should be unique to make debugging easy */

  /* buffers */
  int nbufs;                        /* the number of buffers */
  int nbytes;                       /* the size of buffers in bytes */
  int write;                        /* write index, changed by 'put' only */
  int read;                         /* read index, changed by 'get' only */
  unsigned int *data[NBIGBUFFERS];  /* circular buffer of pointers */

  /* locks and conditions */
  pthread_mutex_t bb_lock;   /* lock the structure */
  pthread_cond_t bb_cond;    /* full <-> not full condition */

  int cleanup;

  /* for NEW_ROC */
  unsigned int userMemBase[NBIGBUFFERS];
  unsigned int physMemBase[NBIGBUFFERS];

} BIGBUF;




/* control structure for 'coda_net' and 'coda_proc';
 used to pass info between 'coda_roc'/'coda_proc' and 'coda_net' */
typedef struct bignet
{
  int failure; /* failure flag */
  int doclose; /* thread exit request flag */

  /* for coda_net only */
  int port;
  int socket;

  BIGBUF *gbigin;  /* input data buffer */
  BIGBUF *gbigout; /* output data buffer (NULL for 'coda_net') */

  /* general info */
  int token_interval;
  int rocid;
  char host[128];
  char rolname[256];
  char rolparams[128];

  char roc_name[128]; /* CODA name for ROC (for example dc1) */
  char eb_name[128]; /* CODA name for EB (for example EB1) */

} BIGNET;


/* function prototypes */

#ifdef  __cplusplus
extern "C" {
#endif

BIGBUF       *bb_new(int id, int nbufs, int nbytes);
void          bb_delete(BIGBUF **bbp);
void          bb_cleanup(BIGBUF **bbp);
void          bb_init(BIGBUF **bbh);
unsigned int *bb_write(BIGBUF **bbp);
unsigned int *bb_write_(BIGBUF **bbh, int flag);
unsigned int *bb_write_nodelay(BIGBUF **bbh);
unsigned int *bb_write_current(BIGBUF **bbp);
unsigned int *bb_read(BIGBUF **bbp);

#ifdef Linux_vme
void bb_InitChunk(BIGBUF *bbp, int n);
BIGBUF *bb_new_rol1(int id, int nbufs, int nbytes);
unsigned int *bb_get_usermembase(BIGBUF **bbh);
unsigned int *bb_get_physmembase(BIGBUF **bbh);
void bb_dma_free();
void bb_delete1(BIGBUF **bbh);
#endif

void bb_cleanup_pci(BIGBUF **bbh);
unsigned int *bb_read_pci(BIGBUF **bbh);

int usrMem2MemDmaCopy(unsigned int chan, unsigned int *sourceAddr, unsigned int *destAddr, unsigned int nbytes);
int usrMem2MemDmaStart(unsigned int chan, unsigned int *sourceAddr, unsigned int *destAddr, unsigned int nbytes);
int usrDmaDone(unsigned int chan);

#ifdef  __cplusplus
}
#endif


#endif
