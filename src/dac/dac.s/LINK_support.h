
/* LINK_support.h */


#define NPROFMAX 10

typedef struct thread_args *trArg;
typedef struct thread_args
{
  objClass object;
  DATA_LINK link;
} TRARGS;

/* functions */

int bufferSwap(unsigned int *cbuf, int nlongs);
int LINK_sized_read(int fd, char **buf, hrtime_t tprof[NPROFMAX]);
void *handle_link(trArg arg);
DATA_LINK debOpenLink(char *fromname, char *toname, char *tohost, MYSQL *dbsock);
int debCloseLink(DATA_LINK theLink, MYSQL *dbsock);
