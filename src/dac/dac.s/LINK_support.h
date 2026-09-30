
/* LINK_support.h */


#define NPROFMAX 10

/* for LINK_support.c and coda_ebc.c only */

typedef struct data_link *DATA_LINK;
typedef struct data_link
{
  char *name;
  char *linkname;   /* for example 'croctest1->EB5' */
  char *parent;
  pthread_t link_thread;
  pthread_t proc_thread;
  int sock;         /* listening socket (bind) */
  int fd;           /* accepted socket (returned by accept()) */
  int ix;           /* link id from 0 in current daq configuration*/
  char host[100];
  int port;
  int exit;
  int bufCnt;
  
  BIGBUF *gbufin;         /* input data buffer from LINK_sized_read */
  CIRCBUF *roc_queue;     /* output to EB */
  ROLPARAMS *rolP;        /*  */
  
} DATA_LINK_S;


/* functions */

int LINK_sized_read(int fd, int ix, char **buf);
void *handle_link(DATA_LINK theLink);
DATA_LINK debOpenLink(char *fromname, char *toname, char *tohost, MYSQL *dbsock);
int debCloseLink(DATA_LINK theLink, MYSQL *dbsock);
int debForceCloseLink(DATA_LINK theLink, MYSQL *dbsock);
