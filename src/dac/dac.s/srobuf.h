
#ifndef _SROBUF_
#define _SROBUF_

/* srobuf.h */

/* max number of buffers allowed */
#define NSROBUFFERS 1024 /*512*/ /*32*/ /* 16 */

#define SEND_BUF_MARGIN  (SEND_BUF_SIZE/4) /*(MAX_EVENT_LENGTH + 128)*/

typedef struct srobuf
{
  int id; /* buffer id, should be unique to make debugging easy */

  /* buffers */
  int nbufs;                         /* the number of buffers */
  int nbytes;                        /* the size of buffers in bytes */

  int writing;                       /* writing in progress */

  int write;                        /* write index, changed by 'put' only */
  int read;                         /* read index, changed by 'get' only */
  unsigned int *data[NSROBUFFERS];  /* circular buffer of pointers */

  /* locks and conditions */
  pthread_mutex_t sro_lock;   /* lock the structure */
  pthread_cond_t sro_cond;    /* full <-> not full condition */

  int cleanup;

} SROBUF;




/* function prototypes */

#ifdef  __cplusplus
extern "C" {
#endif

SROBUF       *sro_new(int id, int nbufs, int nbytes);
void          sro_delete(SROBUF **bbp);
void          sro_cleanup(SROBUF **bbp);
void          sro_init(SROBUF **bbh);

unsigned int *sro_write(SROBUF **bbp, int *icb_out);
unsigned int *sro_write_current(SROBUF **bbp, int *icb_out);
unsigned int *sro_write_(SROBUF **bbp, int *icb_out);

int           sro_get_check(SROBUF **bbh);
unsigned int *sro_get(SROBUF **bbp, int *icb_out);
unsigned int *sro_get1(SROBUF **bbp, int *icb_out);
  
#ifdef  __cplusplus
}
#endif


#endif
