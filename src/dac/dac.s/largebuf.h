
#ifndef _LARGEBUF_
#define _LARGEBUF_

/* largebuf.h */

/* header */
#define IBUFLEN 0 /* index of buffer length */
#define IBUFNUM 1 /* index of buffer global number to be used in file name extension */
#define IBUFIND 2 /* index of buffer index = buffer local number from 0 to (NUM_ER_BUFS-1) */
#define IBUFEND 3 /* index of end condition */
#define BUFHEAD 4 /* the number of words in big buffer header */

/* max number of buffers allowed */
#define NBUFS 16

#define SEND_BUF_MARGIN  (SEND_BUF_SIZE/4) /*(MAX_EVENT_LENGTH + 128)*/



#define BUF_LOCK(id_m)    pthread_mutex_lock(&lbp->buf_lock[id_m])
#define BUF_UNLOCK(id_m)  pthread_mutex_unlock(&lbp->buf_lock[id_m])
#define BUF_WAIT(id_m)    pthread_cond_wait(&lbp->buf_cond[id_m], &lbp->buf_lock[id_m])
#define BUF_SIGNAL(id_m)  pthread_cond_signal(&lbp->buf_cond[id_m])
/*
#define BUF_LOCK(id_m)    
#define BUF_UNLOCK(id_m)  
#define BUF_WAIT(id_m)    
#define BUF_SIGNAL(id_m)  
*/






/*
#define GLOBAL_LOCK       pthread_mutex_lock(&lbp->global_lock)
#define GLOBAL_UNLOCK     pthread_mutex_unlock(&lbp->global_lock)
#define GLOBAL_WAIT       pthread_cond_wait(&lbp->global_cond, &lbp->global_lock)
#define GLOBAL_SIGNAL     pthread_cond_signal(&lbp->global_cond)
*/
#define GLOBAL_LOCK       
#define GLOBAL_UNLOCK     
#define GLOBAL_WAIT       
#define GLOBAL_SIGNAL     


#define BUF_EMPTY   0
#define BUF_FULL    1
#define BUF_READING 2
#define BUF_WRITING 3

typedef struct largebuf
{
  /* buffers */
  int nbufs;                          /* the number of buffers */
  int nbytes;                         /* the size of buffers in bytes */
  int cleanup;
  unsigned int *data[NBUFS];          /* pointers to buffers */

  int icb; /* writing thread buffer index - assume only one writing thread ! */

  int state[NBUFS];                   /* empty or full */

  /* locks and conditions */
  pthread_mutex_t global_lock;        /* lock whole structure */
  pthread_cond_t global_cond;         /* 'empty' <-> 'full' condition */

  pthread_mutex_t buf_lock[NBUFS];   /* lock the buffer */
  pthread_cond_t buf_cond[NBUFS];    /* 'empty' <-> 'full' condition */

} LARGEBUF;


/* function prototypes */

#ifdef  __cplusplus
extern "C" {
#endif

  LARGEBUF     *lb_new(int id, int nbufs, int nbytes);
  void          lb_delete(LARGEBUF **lbp);
  void          lb_cleanup(LARGEBUF **lbp);
  void          lb_init(LARGEBUF **lbh);
  unsigned int *lb_write_grab(LARGEBUF **lbp);
  unsigned int *lb_write_release(LARGEBUF **lbp);
  unsigned int *lb_read_grab(LARGEBUF **lbp, int icb);
  unsigned int *lb_read_release(LARGEBUF **lbp, int icb);

#ifdef  __cplusplus
}
#endif


#endif
