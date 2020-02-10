
/* LINK_support.h */


#define NPROFMAX 10

/* for LINK_support.c and deb_component.c only - temporary here !!! */

typedef struct data_link *DATA_LINK;
typedef struct data_link
{
  char *name;
  char *linkname;   /* for example 'croctest1->EB5' */
  char *parent;
  pthread_t thread;
  int sock;         /* listening socket (bind) */
  int fd;           /* accepted socket (returned by accept()) */
  char host[100];
  int port;
  int exit;
  int bufCnt;
  CIRCBUF *roc_queue;
} DATA_LINK_S;


/* functions */

int bufferSwap(unsigned int *cbuf, int nlongs);
int LINK_sized_read(int fd, char **buf, hrtime_t tprof[NPROFMAX]);
void *handle_link(DATA_LINK theLink);
DATA_LINK debOpenLink(char *fromname, char *toname, char *tohost, MYSQL *dbsock);
int debCloseLink(DATA_LINK theLink, MYSQL *dbsock);
int debForceCloseLink(DATA_LINK theLink, MYSQL *dbsock);
