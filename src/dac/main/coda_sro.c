
/* coda_sro.c - streaming daq aggregator/dispatcher */

/*
nc -l 7001 > zzz
watch -n 1 tcpClient adcft1vtp vtpStatus | grep NWords
watch -n 1 tcpClient adcft2vtp vtpStatus | grep NWords
 */


//#define PRINT_STAT


//defined in Makefile
//#define DUMMY_BACKEND /* read data from VTPs and discard it */

//#define DUMMY_FB_OUTPUT /* FB does not send anything to output threads */

//#define DUMMY_OUT_OUTPUT /* OUT does not send anything to SPR */


static int debug = 0;

/*insert fake frames if ones from vtp are missing*/
#define FAKE_FRAMES

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(Linux_armv7l)

int
main()
{
  printf("coda_sro is dummy for ARM etc\n");
}

#else


#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <sched.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <stdatomic.h>

#ifdef Linux
#include <sys/prctl.h>
#endif
#if defined __sun||LINUX
#include <dlfcn.h>
#endif

#include "rc.h"
#include "rolInt.h"
#include "da.h"
#include "circbuf.h"
#include "libdb.h"
#include "et_private.h"

#include "srobuf.h"
#include "sroLib.h"


//#define DEBUG




/* input threads data sending timeout - to be adjusted 'on flight' */
pthread_mutex_t dtimeout_lock;
static hrtime_t dtimeout;
#define DTIME_LOCK   pthread_mutex_lock(&dtimeout_lock)
#define DTIME_UNLOCK pthread_mutex_unlock(&dtimeout_lock)


/* big buffers */
static SROBUF  *gbigBUFIN[MAXLINKS];
static SROBUF  *gbigBUFOUT[MAXOUTS];
static uint64_t send_buf_size_bytes;

static int sro_loop_exit = 0;
static pthread_t iTaskROL;
static pthread_attr_t detached_attr;

/*static*/extern objClass localobject;
extern char configname[128]; /* coda_component.c */
extern char *mysql_host; /* coda_component.c */
extern char *expid; /* coda_component.c */
extern char *session; /* coda_component.c */
extern char *configalone; /* coda_component.c */
extern int standalone; /* coda_component.c */
extern float statistics[4];

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

void
sro_loop()
{
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;
  int ii, jj;
  int32_t nevents;
  int64_t nlongs;
  int livetime;
  int64_t nlongs_old[MAXLINKS], nlongs_rate[MAXLINKS], nlongs_rate_total;

#ifdef Linux
  prctl(PR_SET_NAME,"sro_loop");
#endif
  printf("sro_loop started\n");

  /* some initialization */

  for(ii=0; ii<MAXLINKS; ii++) nlongs_old[ii] = 0LL;

  while(1)
  {

    if(sro_loop_exit)
    {
      sro_loop_exit = 0;
      return;
    }
    /*printf("sro_loop...\n");*/

    /* nevents and nlongs are accumulating inside every thread, here we just summing them */
    nevents = 0;
    nlongs = 0LL;
    livetime = 0;
    nlongs_rate_total = 0LL;

    for(ii=0; ii<etp->nthreads; ii++)
    {

      //see above
      nevents += etp->links[ii]->nevents;
 
      nlongs += etp->links[ii]->nlongs;
      livetime += etp->links[ii]->livetime;

      nlongs_rate[ii] = ((etp->links[ii]->nlongs - nlongs_old[ii])*4LL) / 1024LL;
      //#ifdef PRINT_STAT
      if(tcpState == DA_ACTIVE) printf(" [%d] Data rate = %6lld kB/s, livetime = %3d % (%s)\n",ii,nlongs_rate[ii],etp->links[ii]->livetime,etp->name[ii]);
      //#endif
      nlongs_rate[ii] = nlongs_rate[ii] / 1024LL;

      nlongs_rate_total += nlongs_rate[ii];
      nlongs_old[ii] = etp->links[ii]->nlongs;
    }
    object->nevents = nevents;
    object->nlongs = nlongs;
#ifdef PRINT_STAT
    if(etp->nthreads>0) printf("Total rate = %6lld MB/s, average livetime = %3d %\n",nlongs_rate_total,livetime/etp->nthreads);
    printf("\n");
#endif

    sleep(1);
  }

  return;
}

unsigned long long llswap(unsigned long long v)
{
  unsigned long long result = 0;
  result = v >> 32LL;
  result |= v << 32LL;
  return result;
}


#define CODASRO_PORT 7001
#define OUTPUT_PORT  8001


static void
print_codasro_header(EVIO_Record_Header_t *msg)
{
  printf("total_length   0x%08X (%d)\n", msg->total_length, msg->total_length);
  printf("record_counter 0x%08X (%d)\n", msg->record_counter, msg->record_counter);
  printf("header_length  0x%08X (%d)\n", msg->header_length, msg->header_length);
  printf("event_count    0x%08X (%d)\n", msg->event_count, msg->event_count);
  printf("roc_id         0x%08X (%d)\n", msg->roc_id, msg->roc_id);
  printf("evio_version   0x%08X (%d)\n", msg->evio_version, msg->evio_version);
  printf("frame_counter  0x%08X (%d)\n", msg->frame_counter, msg->frame_counter);
  printf("magic          0x%08X (%d)\n", msg->magic, msg->magic);
}



/* one frame = 65536us */


#define IN_WRITE_BUFFER \
    /* write buffer if it is half full, OR (timeout and not empty); dabufp[0] in words, 'send_buf_size_bytes' in bytes !!! */ \
    /*printf("[%d] IN: info: check conditions for writing\n",ithread);*/	\
    if( (dabufp[0] > (send_buf_size_bytes/4/2) ) || ( ((frame_number%NFRAMES)==0) && (dabufp[0]>1) ) ) \
    { \
      /*if( dabufp[0] > (send_buf_size_bytes/4/2) ) printf("[%d] IN: next buf on half full (dt=%d, dtimeout=%d) (len=%d)\n",ithread,timeout-currenttime,dtimeout,dabufp[0]);*/ \
      /*else                                    printf("[%d] IN: next buf on frame_number=%d (len=%d)\n",ithread,frame_number,dabufp[0]);*/ \
      /* switching output buffer */ \
      /*printf("[%d] IN: filled output buffer[%d] with %d bytes, switching to next one\n",ithread,icb,dabufp[0]);fflush(stdout);*/ \
      /*printf("[%d] IN: befor sro_write (icb=%d, len=%d)\n",ithread,icb,dabufp[0]);fflush(stdout);*/ \
      outptr = dabufp = sro_write(&(theLink->outbuf), &icb); \
      /*printf("[%d] IN: after sro_write\n",ithread);fflush(stdout);*/  \
      if(dabufp == NULL) \
      { \
        printf("[%d] IN: info from sro_write: RETURN 0\n",ithread); fflush(stdout); \
        goto exit; \
      } \
      else \
      { \
        dabufp[0] = 1; /* first word is counted as well, so buffer length is inclusive !!! */ \
        outptr ++; \
        /*printf("[%d] IN: info - grabbed next circular dabufp=0x%llx (outptr=0x%llx), icb=%d\n",ithread,dabufp,outptr,icb);fflush(stdout);*/ \
      } \
    }


void *
sro_input_thread(LINKS theLink)
{
  objClass object = localobject;
  struct sockaddr_in sin_codasro;
  int input_socket;
  int ii, jj, kk, ix, icb, status;
  char *ch;
  unsigned int *ptr;
  size_t ret;
  struct sockaddr_in from;
  int slen = sizeof(from);
  int codasro_connection = 0;
  int wave_len, len, val, slot, slot_idx, opt;
  int port_in = CODASRO_PORT;
  
  int ithread = theLink->ithread;
  char *name = theLink->name;
  char *host = theLink->host;

  EVIO_Record_Header_t codasro_msg, last_codasro_msg;
  unsigned int fake_payload[10];

  unsigned int *dabufp; /* always points to the beginning of the buffer */
  unsigned int *outptr; /* current hit pointer in the buffer */
  unsigned int *in, *out;
  unsigned int payload_length, fake_payload_length;
  unsigned int frame_number, frame_number_up, frame_number_old = 0;

  char thread_name[128];
  char host_in[80];

  int cnt = 0;
  int nreads = 0;
  int nwords = 0;
  int nwords_lost = 0;

  //hrtime_t currenttime;
  /*time_t*/hrtime_t timeout;

  unsigned long long record_counter_total = 0LL;
  unsigned long long record_counter_received = 0LL;
  unsigned long long record_counter_at_last_print = 0LL;
  unsigned long long livetime = 0LL;

  MYSQL *dbsock;
  char tmpp[1000];

  
  sprintf(thread_name,"si_%s\n",name);
#ifdef Linux
  prctl(PR_SET_NAME,thread_name);
#endif

  printf("\n[%d] IN: sro_input_thread: name >%s<, host >%s<\n\n",ithread,name,host); 
  printf("[%d] IN: CODA SRC sockets initialization =====\n",ithread);

  input_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(input_socket < 0)
  {
    perror("Can't open socket ");
    pthread_exit(0);
  }
  else
  {
    printf("[%d] input_socket=%d\n",ithread,input_socket);
  }




  /*TEST: allow port reuse, trying to fix vtp connection problem ...*/
  opt = 1;
  if(setsockopt(input_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
  {
    perror("setsockopt failed");
    exit(EXIT_FAILURE);
  }
  /*TEST*/


  
  bzero(&sin_codasro, sizeof(sin_codasro));
  sin_codasro.sin_family = AF_INET;
  sin_codasro.sin_addr.s_addr = htonl(INADDR_ANY);
  printf("[%d] IN: trying port_in = %d ..\n",ithread,port_in);fflush(stdout);
  sin_codasro.sin_port = htons(port_in);

  while(bind(input_socket, (struct sockaddr *) &sin_codasro, sizeof(sin_codasro)) < 0)
  {
    //perror("bind error");
    /* try another port (just increment on one) */
    port_in ++;
    printf("[%d] IN: trying port_in = %d ..\n",ithread,port_in);fflush(stdout);
    if((port_in-CODASRO_PORT) > 100)
    {
      close(input_socket); 
      printf("[%d] IN: %s exits -> ", "sro_input_thread2: ",ithread);fflush(stdout);
      perror("bind error");
      pthread_exit(0);
    }
    sin_codasro.sin_port = htons(port_in);
  }
  printf("\t[%d] binding port %d for incoming connection from CODASRO client\n",ithread,port_in);
  theLink->port_in = port_in;
  theLink->socket_in = input_socket;


  /* update 'Streamin' table in 'daq_daq' database */
  /* obtain destination hostname - HAVE TO GET IT FROM DATABASE, maybe be it is not default hostname */
  dbsock = dbConnect(mysql_host, expid);
  if(dbsock==NULL)
  {
    printf("[%d] IN: cannot connect to the database - exit\n",ithread);fflush(stdout);
    pthread_exit(0);
  }
  sprintf(tmpp,"SELECT host FROM process WHERE name='%s'",object->name);
  if(dbGetStr(dbsock, tmpp, host_in)==CODA_ERROR)
  {
    printf("[%d] IN: cannot get host_in from database - exit\n",ithread);fflush(stdout);
    pthread_exit(0);
  }
  printf("[%d] IN: host_in is >%s<\n",ithread,host_in);fflush(stdout);
  dbDisconnect(dbsock);

  printf("[%d] IN: update Streamin table: name >%s<, host >%s<, port_in=%d, host_in >%s<\n",ithread,name,host,port_in,host_in);fflush(stdout);
  codaStreaminTableUpdate(name, host, port_in, host_in);
  /* update database flag to let following threads know they can start */
  theLink->database_updated = 1;

  
  /* listening on input socket */
  if(listen(input_socket, 1) < 0)
  {
    perror("listen failed ");
    pthread_exit(0);
  }







  
  printf("\n==========================================\n");
  printf("[%d] IN: Accepting CODASRO Connection ...\n\n",ithread);

accept1:
  bzero((char *) &from, slen);
  //printf("[%d] IN: 11 slen=%d\n",ithread,slen);fflush(stdout);
  codasro_connection = accept(input_socket, (struct sockaddr *) &from, (socklen_t *) &slen);
  //printf("[%d] IN: 12 codasro_connection=%d\n",ithread,codasro_connection);fflush(stdout);
  if(codasro_connection > 0)
  {
    theLink->fd_in = codasro_connection;
    printf("[%d] IN: We've got a connection from %s\n",ithread, inet_ntoa((struct in_addr) from.sin_addr));
  }
  else
  {
    printf("accept: errno=%d\n",errno);
    perror ("accept: ");
    printf("Failed to make connection on CODASRO port.\n");
    pthread_exit(0);
  }

  if(!strncmp(inet_ntoa(from.sin_addr),"129.57.71.",10))
  {
    printf("[%d] IN: CC SCAN !!!!!!!!!!!!!!!!!!!\n",ithread);
    goto accept1;
  }

//  i = 1;
//  setsockopt(codasro_connection, IPPROTO_TCP, TCP_QUICKACK, (void *)&i, sizeof(i));

/* read 'emuData[]' if inserted
  printf("[%d] IN: Reading CODASRO connection header ...\n",ithread);
  read_socket(codasro_connection, (unsigned char *)&val, sizeof(val), 1, ithread);
  printf("  [%d] IN: CODA Streaming Magic ID: 0x%08X\n",ithread, val);
  read_socket(codasro_connection, (unsigned char *)&val, sizeof(val), 1, ithread);
  printf("  [%d] IN: CODA Streaming Version: 0x%08X\n",ithread, val);
*/

  printf("[%d] IN: starting sro_input_thread ...\n",ithread);

  outptr = dabufp = sro_write_current(&(theLink->outbuf), &icb);
  if(dabufp == NULL)
  {
    printf("[%d] IN: error in sro_write_current: FAILED\n",ithread);
    goto exit;
  }
  else
  {
    dabufp[0] = 1; /* inclusive length */
    outptr ++;
    printf("[%d] IN: info - grabbed next circular dabufp=0x%llx (outptr=0x%llx), icb=%d\n",ithread,dabufp,outptr,icb);fflush(stdout); \
  }


/* read everything for debugging purposes
payload_length = 128*2048;
len = read_socket(codasro_connection, (unsigned char *)theLink->CODASRO_Payload, payload_length, 1, ithread);
ptr = (unsigned int *)theLink->CODASRO_Payload;
printf("\n11\nlen=%d\n",len);
 for(jj=0; jj<len/4; jj++) printf(" [%2d] 0x%08x %d\n",jj,ptr[jj],ptr[jj]);
printf("12\n\n");
exit(0);
*/


  /* loop, until exit or error */
  while(theLink->exit==0)
  {
    //printf("[%d] IN: theLink->exit=%d\n",ithread,theLink->exit);fflush(stdout);
#ifdef DEBUG
    printf("[%d] IN: Reading cMsg header ...\n",ithread);
#endif
    len = read_socket(codasro_connection, (unsigned char *)&val, sizeof(val), 1, ithread);
    if(len<=0)
    {
      printf("[%d] IN: codasro_connection terminated. Exiting...\n",ithread);fflush(stdout);
      break;
    }
#ifdef DEBUG
    printf("  [%d] IN: cMsg header length = %d\n",ithread, val);
#endif
    len = read_socket(codasro_connection, (unsigned char *)&val, sizeof(val), 1, ithread);
    if(len<=0)
    {
      printf("[%d] IN: codasro_connection terminated. Exiting...\n",ithread);fflush(stdout);
      break;
    }
    //printf("  [%d] IN: cMsg: length of the following buffer in bytes = %d\n",ithread, val);


    //printf("[%d] IN: info - reading record header, connection=%d ..\n",ithread,codasro_connection);fflush(stdout); // input threads waits here before Go
    len = read_socket(codasro_connection, (unsigned char *)&codasro_msg, RECORD_HEADER_LENGTH*4, 1, ithread);
    if(len<=0)
    {
      printf("[%d] IN: codasro_connection terminated. Exiting...\n",ithread);fflush(stdout);
      break;
    }
    //printf("[%d] IN: .. got codasro_msg, connection=%d, len=%d bytes\n",ithread,codasro_connection,len);fflush(stdout);
    if(len != RECORD_HEADER_LENGTH*4)
    {
      printf("[%d] IN: invalid receive record header length = %d\n",ithread, len);
      break;
    }

    /*check record header*/
    if(codasro_msg.magic != MAGIC)
    {
      printf("[%d] IN: codasro_msg.magic != MAGIC, received 0x%08X\n",ithread, codasro_msg.magic);
      printf("[%d] IN: Current:\n",ithread);
      print_codasro_header(&codasro_msg);
      printf("[%d] IN: Last:\n",ithread);
      print_codasro_header(&last_codasro_msg);fflush(stdout);
      break;
    }


    payload_length = codasro_msg.total_length - RECORD_HEADER_LENGTH; /* in 32bit words */
    if(payload_length > sizeof(theLink->CODASRO_Payload))
    {
      printf("[%d] IN: payload_length > max, want to received %u\n",ithread, payload_length);fflush(stdout);
      break;
    }
    theLink->nlongs += RECORD_HEADER_LENGTH;

    // Read CODASRO payload
    //printf("[%d] IN: reading payload .. (payload_length=%d bytes)\n",ithread,payload_length*4);fflush(stdout);
    len = read_socket(codasro_connection, (unsigned char *)theLink->CODASRO_Payload, payload_length*4, 1, ithread);
    //printf("[%d] IN: .. got payload, len=%d bytes\n",ithread,len);fflush(stdout);

    if(len <= 0)
    {
      printf("[%d] IN: codasro_connection terminated. Exiting...\n",ithread);fflush(stdout);
      break;
    }
    if(len != payload_length*4)
    {
      printf("[%d] IN: invalid1 receive payload_length: %u bytes\n",ithread, len);fflush(stdout);
      break;
    }

    /*check payload data*/
    if((payload_length-1) != theLink->CODASRO_Payload[0])
    {
      printf("[%d] IN: invalid2 receive payload_length: %u words\n",ithread, theLink->CODASRO_Payload[0]);fflush(stdout);
      break;
    }

    
    
    /* copy frame# from payload to record header */
    codasro_msg.frame_counter = theLink->CODASRO_Payload[5];


    
    //print_codasro_header(&codasro_msg);
    //printf("payload: %d %08x\n\n",theLink->CODASRO_Payload[0],theLink->CODASRO_Payload[1]);

    frame_number = frame_number_up = codasro_msg.frame_counter;

#ifdef FAKE_FRAMES
    /* check frame_number increment, must be +1; if we lost frame(s), fill in and insert fake record headers */
    if(frame_number_old>0)
    {
      if(frame_number_up != (frame_number_old+1))
      {
        /*printf("[%d] lost %d frames\n",ithread,frame_number_up-frame_number_old);*/
        //printf("[%d] LOST FRAMES ==> frame_number_up = %6d, frame_number_old = %6d -> %6d frames lost\n",
        //  ithread,frame_number_up,frame_number_old,frame_number_up-frame_number_old);

	/* form fake payload */
	fake_payload_length = 9;                               /*total (inclusive) payload bank length*/
        fake_payload[0] = 8;                                   /*payload bank header 1st word: exclusive bank length*/
        fake_payload[1] = theLink->CODASRO_Payload[1] | 0x80;  /*payload bank header 2nd word: set ERROR flag in Stream Status */
        fake_payload[2] = 6;                                   /*stream info bank header 1st word: exclusive bank length*/
        fake_payload[3] = (0xff30<<16) | (0x20<<8) | 0x80;     /*stream info bank header 2nd word*/
        fake_payload[4] = (0x31<<24) | (0x1<<16) | 3;          /*time slice segment header*/
        fake_payload[5] = last_codasro_msg.frame_counter;      /*frame number - to be updated in the loop below*/
        fake_payload[6] = last_codasro_msg.frame_counter<<16;  /*timestamp_l (ASSUME IT IS FRAME_NUMBER<<16 !!!!! NEEDED BY PROCESSING THREAD FOR DOUBLE CHECK)*/
        fake_payload[7] = 0;                                   /*timestamp_h*/
        fake_payload[8] = (0x41<<24) | (0x85<<16) | 0;         /*aggregation info segment header*/

	
        //printf("[%d] IN: started fake frames insertion, first frame inserted is %d\n",ithread,fake_payload[5]+1);

        frame_number = fake_payload[5];
        while(frame_number<(frame_number_up-1))
        {
	  /*update frame number*/
	  fake_payload[5] ++;
	  frame_number = fake_payload[5];
          fake_payload[6] = frame_number<<16;

          /* copy fake_payload into output buffers */
          in = (unsigned int *)&fake_payload[0];
          out = outptr;
          for(kk=0; kk<fake_payload_length; kk++) *out++ = *in++;
          dabufp[0] += fake_payload_length; /* update total buffer length (ints) */
          outptr += fake_payload_length; /* update data pointer */
	  
#ifdef DUMMY_BACKEND
          /*re-write the same buffer*/
          if( (dabufp[0] > (send_buf_size_bytes/4/2) ) || ( ((frame_number%NFRAMES)==0) && (dabufp[0]>1) ) )
          {
            outptr = dabufp;
            dabufp[0] = 1;
            outptr ++;
          }
#else
          IN_WRITE_BUFFER; 
#endif
        } /*while*/

        //printf("[%d] IN: finished fake frames insertion, last frame inserted is %d\n",ithread,frame_number);
      }
    }
    frame_number = frame_number_old = frame_number_up;
    memcpy(&last_codasro_msg, &codasro_msg, RECORD_HEADER_LENGTH*4);
#endif

    
    /* copy payload into output buffers */
    in = (unsigned int *)&theLink->CODASRO_Payload[0];
    out = outptr;

    for(kk=0; kk<payload_length; kk++)
    {
      *out++ = *in++;
    }
    dabufp[0] += payload_length; /* update total buffer length (ints) */
    outptr += payload_length; /* update data pointer */

    theLink->nlongs += payload_length;
    //printf("[%d] IN: payload=%d, nlongs=%lld\n",ithread,payload_length,theLink->nlongs);

    nreads ++;
    if(nreads>0/*12500*/)
    {
      nreads = 0;
      theLink->nevents ++;
    }



    /* print some statistics */
    record_counter_received ++;
    record_counter_total = codasro_msg.record_counter - record_counter_at_last_print;

    cnt ++;
    if(((codasro_msg.frame_counter+1)%10000)==0/*cnt>100*/)
    {
      cnt=0;

      //livetime = (record_counter_received*100LL) / record_counter_total;
      //printf("[%d] IN: COUNTERS %6d %6d\n",ithread,codasro_msg.record_counter,codasro_msg.frame_counter+1);
      livetime = (codasro_msg.record_counter*100LL) / (codasro_msg.frame_counter+1);


      theLink->livetime = (int)livetime;
      nwords = 0;
      nwords_lost = 0;
      record_counter_received = 0LL;
      record_counter_at_last_print = codasro_msg.record_counter;
    }



#ifdef DUMMY_BACKEND
    /*re-write the same buffer*/
    if( (dabufp[0] > (send_buf_size_bytes/4/2) ) || ( ((frame_number%NFRAMES)==0) && (dabufp[0]>1) ) )
    {
      outptr = dabufp;
      dabufp[0] = 1;
      outptr ++;
    }
#else
    IN_WRITE_BUFFER;
#endif

  }

  printf("[%d] IN: Exited while() loop\n",ithread);

exit:

  printf("[%d] IN: Executing Exit commands\n",ithread);
  close(codasro_connection);
  close(input_socket);
  theLink->exit = -1;

  pthread_exit(0);
}





/************************************************************************************/
/* gets frames from inbuf's, and place frames with the same timestamp into outbuf's */

pthread_mutex_t fb_lock = PTHREAD_MUTEX_INITIALIZER;;
#define FB_LOCK   pthread_mutex_lock(&fb_lock)
#define FB_UNLOCK pthread_mutex_unlock(&fb_lock)

pthread_cond_t fb_cond = PTHREAD_COND_INITIALIZER;
#define FB_WAIT   pthread_cond_wait(&fb_cond, &fb_lock)
//#define FB_SIGNAL pthread_cond_signal(&fb_cond)
#define FB_SIGNAL pthread_cond_broadcast(&fb_cond)

int global_sequence = 0;




typedef struct fbstruct
{
  _Atomic int32_t  frame_num;   /*frame number currently being routed to that output (can be -1, this is why it is 'int' and not 'uint'*/
  _Atomic uint64_t inputs_mask; /*input streams mask for the current frame 'so far'*/
  
} FBSTRUCT;
static FBSTRUCT fbstr[MAXOUTS];

static unsigned int *dabufpo[MAXOUTS];
static atomic_int next_index[MAXOUTS];

static int icbout[MAXOUTS];
static /*time_t*/hrtime_t fbtimeout[MAXOUTS];
static uint64_t mask_needed_global;
static hrtime_t currenttime[MAXOUTS];

static int aggr_len;

void *
sro_fb_thread(FBS theFB)
{
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;
  char name[80];
  int ii, jj, kk, lentot, slot, res, ret, status, error, nwords, itmp, my_index, my_len;
  int ithread = theFB->ithread;
  int iiout = 0;
  unsigned int *dabufpi[MAXLINKS];
  int icb[MAXLINKS], index[MAXLINKS], len[MAXLINKS];
  unsigned int frame_num, frame_len, *frame_ptr;
  unsigned int buf[2];
  int frame0[10], frame1[10];
  uint64_t mask_needed;
  int copied, minus_one;
  time_t startloop;
  
  sprintf(name,"sf_%03d",ithread);
#ifdef Linux
  prctl(PR_SET_NAME,name);
#endif

  printf("\n[%d] FB: reached\n\n",ithread);

  mask_needed = theFB->mask_needed;
  printf("\n[%d] FB: mask_needed=0x%08x\n\n",ithread,mask_needed);
  
  for(ii=0; ii<MAXLINKS; ii++)
  {
    dabufpi[ii] = NULL;
    len[ii] = 0;
    index[ii] = 1;
  }

  printf("\n[%d] FB: ready for frame building ...\n\n",ithread);
  while(theFB->exit==0)
  {

    for(ii=0; ii<etp->nthreads; ii++)
    {

      /*process only inputs streams required by our mask*/
      if( (mask_needed & (1ULL<<ii)) == 0 )
      {
        //printf("[%d] mask_needed=0x%llx, ii=%d - not in our mask, skip\n",ithread,mask_needed,ii);
        continue;
      }
      //printf("[%d] FB: reading stream %d\n",ithread,ii);
      
      if(index[ii] >= len[ii]) /* if we finished reading current buffer, grab next one */
      {
        /* get input buffer from input thread */ 
	//printf("[%d] FB: befor sro_get() (because index=%d >= len=%d)\n",ithread,index[ii],len[ii]);fflush(stdout);
        dabufpi[ii] = sro_get(&(theFB->inbuf[ii]), &icb[ii]);
	//printf("[%d] FB: after sro_get(): icb=%d\n",ithread,icb[ii]);fflush(stdout);
        if(dabufpi[ii] == NULL)
	{
          printf("[%d] FB: ERROR in sro_get() - exit\n",ithread);
          goto exit;
	}
        index[ii] = 1;
        len[ii] = dabufpi[ii][0]; /* inbuf[] length in words, excluding first word */
        //printf("[%d] FB: got inbuf[], len=%d, index=%d\n",ithread,len[ii],index[ii]);fflush(stdout);

	//printf("11\n");fflush(stdout);
	//buffercheck1(dabufpi[ii],1);
	//printf("12\n");fflush(stdout);

      }
      else /* still have data in current buffer */
      {
        //index[ii] += 0;
        //printf("[%d] FB: still in the same inbuf[], len=%d, index=%d\n",ithread,len[ii],index[ii]);fflush(stdout);
      }

      /* get next frame from stream 'ii' */
      jj = index[ii];

      /*remember frame number, frame length and frame pointer*/	     
      frame_len = dabufpi[ii][jj] + 1; /*the number of words in frame*/
      frame_num = dabufpi[ii][jj+5];  /*frame number*/
      frame_ptr = (unsigned int *)&dabufpi[ii][jj]; /*pointer to the frame*/




      
      /***************************/
      /*output scheduler/calendar*/

      /* 'iiout' is the index of the output stream we will use to send current frame, lets call it 'desired' iiout */
      iiout = frame_num % MAXOUTS; /*round-robin*/
      if(debug) printf("== [%d] FB: stream %d, frame_num=%d -> we want iiout=%d\n",ithread,ii,frame_num,iiout);      
      /*...*/

      

      /*loop until we copy our current frame to desired 'iiout'*/
      startloop = time(NULL);
      while(theFB->exit==0)
      {
        int value0;
	uint64_t value1;
	
	FB_LOCK;
	while(1)
	{
          if(theFB->exit!=0) break;

	  value0 = atomic_load(&fbstr[iiout].frame_num);   /*get 'frame_num' assigned to the desired 'iiout'*/
          if(debug) printf("\n[%d] ==1== want iiout=%d (my frame_num=%d, fbstr[iiout].frame_num=%d, fbstr[iiout].inputs_mask=0x%llx) =====\n",ithread,iiout,frame_num,value0,value1);fflush(stdout);
          if( (value0>-1) && (frame_num>=(value0+MAXOUTS)) )
	  {
	    if(debug) printf("[%d] FULL CIRCLE IN, iiout=%d (frame_num=%d, fbstr[iiout].frame_num=%d, fbstr[iiout].inputs_mask=0x%llx)\n",ithread,iiout,frame_num,value0,value1);
	    FB_WAIT;
	  }
	  else
	  {
	    break;
	  }
	}
	if(debug) printf("[%d] FULL CIRCLE OUT OF LOOP, iiout=%d (frame_num=%d, fbstr[iiout].frame_num=%d, fbstr[iiout].inputs_mask=0x%llx)\n",ithread,iiout,frame_num,value0,value1);
	FB_UNLOCK;
        if(theFB->exit!=0) break;
	
 	value1 = atomic_load(&fbstr[iiout].inputs_mask); /*get current 'inputs_mask' in  desired 'iiout'*/
        if(value1 == mask_needed_global)
	{
	  printf("[%d] ERROR: FULL MASK(2) SHOULD NOT HAPPEN AT THAT MOMENT !!!\n",ithread);
	}

        /*********************************************************/
        /* if 'iiout' is free, claim it and copy our chunk there */
        minus_one = -1;
	copied = 0;
	uint64_t mask_needed_global_tmp = mask_needed_global;

        if( atomic_compare_exchange_strong(&fbstr[iiout].frame_num, &minus_one, frame_num) ) //if fbstr[iiout].frame_num==minus_one', it does 'fbstr[iiout].frame_num = frame_num' and returns 'true'
	{
	  my_index = atomic_fetch_add(&next_index[iiout], frame_len);
	  if(debug) printf("[%d] 2 iiout=%d, my_index=%d, frame_len=%d, claim and coping ...\n",ithread,iiout,my_index,frame_len);fflush(stdout);
          memcpy(&dabufpo[iiout][my_index], frame_ptr, frame_len*4); /* copy current frame to 'iiout' buffer */
          if(debug) printf("[%d] 2 iiout=%d, my_index=%d, frame_len=%d, copied !\n",ithread,iiout,my_index,frame_len);fflush(stdout);
	  atomic_fetch_or(&fbstr[iiout].inputs_mask, (1ULL << ii));
          if(debug) printf("[%d] 2 now: iiout=%d (my frame_num=%d, fbstr[iiout].frame_num=%d, fbstr[iiout].inputs_mask=0x%llx) =====\n",ithread,iiout,frame_num,fbstr[iiout].frame_num,fbstr[iiout].inputs_mask);fflush(stdout);
          copied = 1;
	}

	/********************************************************/
	/* 'iiout' already accepting frames with my 'frame_num' */
	else if( (value0 = atomic_load(&fbstr[iiout].frame_num)) == frame_num )
	{
          /*check if our bit already set, it should never happen*/
	  uint64_t value1 = atomic_load(&fbstr[iiout].inputs_mask);
          if( value1 & (1ULL<<ii) )
	  {
	    printf("[%d] WARN: bit %d already set  - SHOULD NEVER HAPPEN (value0=%d, value1=0x%llx)\n",ithread,ii,value0,value1);
	    continue;
	  }

	  my_index = atomic_fetch_add(&next_index[iiout], frame_len); /*claim index in the output buffer ('next_index[iiout]' increased to be used by next thread)*/ 
	  if(debug) printf("[%d] 1 iiout=%d, my_index=%d, frame_len=%d, coping ...\n",ithread,iiout,my_index,frame_len);fflush(stdout);
          memcpy(&dabufpo[iiout][my_index], frame_ptr, frame_len*4); /* copy current frame to 'iiout' buffer */
          if(debug) printf("[%d] 1 iiout=%d, my_index=%d, frame_len=%d, copied !\n",ithread,iiout,my_index,frame_len);fflush(stdout);
	  atomic_fetch_or(&fbstr[iiout].inputs_mask, (1ULL << ii)); /*update inputs_mask*/
          if(debug) printf("[%d] 1 now: iiout=%d (my frame_num=%d, fbstr[iiout].frame_num=%d, fbstr[iiout].inputs_mask=0x%llx) =====\n",ithread,iiout,frame_num,fbstr[iiout].frame_num,fbstr[iiout].inputs_mask);fflush(stdout);
          copied = 1;
	}
	
	if(copied)
	{
	  mask_needed_global_tmp = mask_needed_global;
          /*******************************************************/
          /* if 'iiout' has completed set of frames, send it out */
          if( atomic_compare_exchange_strong(&fbstr[iiout].inputs_mask, &mask_needed_global_tmp, 0ULL) ) //if fbstr[iiout].inputs_mask==mask_needed_global_tmp', it does 'fbstr[iiout].inputs_mask = 0ULL' and returns 'true')
          {
            if(debug) printf("    [%d] FB: stream %d, frame_num=%d [iiout_frame=%d, iiout_mask=0x%llx] -> sending iiout=%d\n",ithread,ii,frame_num,fbstr[iiout].frame_num,fbstr[iiout].inputs_mask,iiout);
            /*send 'iiout' output buffer and get new one on certain conditions*/
            currenttime[iiout] = gethrtime();
            if( ( (next_index[iiout] > (send_buf_size_bytes/4/2) ) || ((currenttime[iiout] > fbtimeout[iiout])&&(next_index[iiout]>1)) ) )
            {
#ifdef DUMMY_FB_OUTPUT
              next_index[iiout] = 1; /* set index to the second word - first word reserved for the total length */
#else
	      dabufpo[iiout][0] = next_index[iiout]; /*update 'big' buffer total length, it is equal to the current next_index (first word is counted as well) */
              if(debug) printf("[%d] FB: iiout=%d: calling sro_write_() (totlen=%d %d)\n",ithread,iiout,dabufpo[iiout][0],next_index[iiout]);fflush(stdout);
              dabufpo[iiout] = sro_write_(&(theFB->outbuf[iiout]), &icbout[iiout]);
              if(dabufpo[iiout] == NULL)
              {
                printf("[%d] FB: INFO from sro_write_: RETURN 0\n",ithread);
                goto exit;
              }
              else
              {
                next_index[iiout] = 1; /* set index to the second word - first word reserved for the total length */
              }
#endif	      
              fbtimeout[iiout] = currenttime[iiout] + 1*100000;
            }
	    int iii = atomic_fetch_add(&next_index[iiout],aggr_len);
	    dabufpo[iiout][iii+8] = (0x42<<24) | (0x1<<16) | etp->nthreads; /* fill 8rd word in aggregated header, will be used by coda_spr.c */
            atomic_store(&fbstr[iiout].frame_num, -1); /*mark this output as 'free'*/
	    if(debug) printf("    [%d] FB: iiout=%d sent !\n",ithread,iiout);
            FB_SIGNAL;
	    if(debug) printf("    [%d] SENT SIGNAL\n",ithread);
	  }
          break;
	}

	if(time(NULL)>(startloop+1))
	{
          printf("[%d] TIMEOUT FB: stream %d, frame_num=%d [iiout_frame=%d, iiout_mask=0x%llx] -> iiout=%d -> sleeping a little ...\n",ithread,ii,frame_num,fbstr[iiout].frame_num,fbstr[iiout].inputs_mask,iiout);
	}
	
        //if(debug) printf("[%d] FB: stream %d, frame_num=%d [iiout_frame=%d, iiout_mask=0x%llx] -> iiout=%d -> sleeping a little ...\n",ithread,ii,frame_num,fbstr[iiout].frame_num,fbstr[iiout].inputs_mask,iiout);
	//microsleep(1000);
      }
      
      /*output scheduler/calendar*/
      /***************************/

      
      index[ii] += frame_len; /*set stream 'ii' pointer to the next frame*/

    } /*for(ii=0; ii<etp->nthreads; ii++)*/

    
  } /*while(theFB->exit==0)*/


exit:

  printf("[%d] sro_fb_thread: received 'exit'\n",ithread);
  theFB->exit = -1;

  pthread_exit(0);
}







/*****************************************************************/
/* gets data from frame builder and send it to processing thread */

void *
sro_output_thread(OUT theOut)
{
  int ii, jj, len, slot, res, ret, icb0, icb, slot_len, status, error, lenerr;
  int type;
  char name[80];
  int ithread = theOut->ithread;
  unsigned int *dabufpi;
  int input_socket, input_connect;
  struct sockaddr_in from;
  int slen = sizeof(from);
  struct hostent *h;
  struct sockaddr_in sin_output;
  int connection = 0;
  int port_out = 0;
  int output_socket = 0;
  int flags = 0/*MSG_DONTWAIT*/;

  sprintf(name,"so_%03d",ithread);
#ifdef Linux
  prctl(PR_SET_NAME,name);
#endif




  /**************************/
  /* opening OUTPUT streams */

  printf("\n=========================================================\n");
  printf("[%d] OUT: output sockets initialization =====\n\n",ithread);



  /***************************/
  /* preparing OUTPUT stream */

  port_out = OUTPUT_PORT;
  printf("[%d] OUT: trying to get port_out %d\n",ithread,port_out);

  output_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(output_socket < 0)
  {
    perror("Can't open socket ");
    pthread_exit(0);
  }

  
  bzero(&sin_output, sizeof(sin_output));
  sin_output.sin_family = AF_INET;
  sin_output.sin_addr.s_addr = htonl(INADDR_ANY);
  sin_output.sin_port = htons(port_out);

  while(bind(output_socket, (struct sockaddr *) &sin_output, sizeof(sin_output)) < 0)
  {
    /* try another port (just increment on one) */
    port_out ++;
    if((port_out-OUTPUT_PORT) > 200)
    {
      close(output_socket); 
      perror("bind error  ");
      pthread_exit(0);
    }
    sin_output.sin_port = htons(port_out);
  }




  
  gethostname(theOut->host,80);  /* obtain our hostname - ERROR ! */




  
  printf("\t[%d] OUT: listening on host '%s' port %d for incoming connection from output client (processing unit)\n",
            ithread,theOut->host,port_out);
  theOut->socket_out = output_socket;
  if(listen(output_socket, 1) < 0)
  {
    perror("listen failed ");
    pthread_exit(0);
  }

  /* update 'Streamout' table in 'daq_daq' database */
  codaStreamoutTableUpdate(name, theOut->host, port_out);


  /* that will tell main thread that we are ready; do it AFTER daq_daq/Streamout is updated ! */
  theOut->port_out = port_out;



  /* accepting OUTPUT stream ONLY if processing threads exists */

#if 1
accept2:

  bzero((char *) &from, slen);
  connection = accept(theOut->socket_out, (struct sockaddr *) &from, (socklen_t *) &slen);
  if(connection > 0)
  {
    theOut->fd_out = connection;
    printf("[%d] OUT: We've got a connection from %s\n",ithread,inet_ntoa((struct in_addr) from.sin_addr));fflush(stdout);
  }
  else
  {
    printf("[%d] OUT: Failed to make connection on OUTPUT port.\n",ithread);fflush(stdout);
    pthread_exit(0);
  }

  if(!strncmp(inet_ntoa(from.sin_addr),"129.57.71.",10))
  {
    printf("OUT: CC SCAN !!!!!!!!!!!!!!!!!!! - IGNORED\n");fflush(stdout);
    goto accept2;
  }

  /* set SO_KEEPALIVE so we can catch when socket goes down even if we do not send anything */
  {
    int optval, lbytes;

    optval = 1; /* 1-yes, 0-no */
    if(setsockopt(connection, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0)
    {
      printf("[%d] OUT: setsockopt(,,SO_KEEPALIVE,,) failed\n",ithread);
      goto exit;
    }

    optval = 0;
    lbytes = 4;
    getsockopt(connection, SOL_SOCKET, SO_KEEPALIVE, (int *) &optval, &lbytes);
    //printf("[%d] OUT: getsockopt(,,SO_KEEPALIVE,,) returns keepAlive = %d\n",ithread,optval);
  }
#endif



  /* loop until get exit flag */
  while(theOut->exit==0)
  {



#if 0
    /* check if socket still UP */
    error = 0;
    lenerr = sizeof (error);
    res = 0;
    res = getsockopt (connection, SOL_SOCKET, SO_ERROR, &error, &lenerr);
    if(res<0)
    {
      perror("getsockopt:");
      goto exit; //goto accept2;
    }
    if(error!=0)
    {
      printf("[%d] OUT: getsockopt() returns error=%d - goto accept()\n",ithread,error);
      goto exit; //goto accept2;
    }
#endif



    /* get input buffer from fb thread */ 
    //printf("[%d] OUT: befor sro_get(%p,,)\n",ithread,theOut->inbuf);fflush(stdout);
    dabufpi = sro_get1(&(theOut->inbuf), &icb);
    //printf("[%d] OUT: after sro_get: icb=%d\n",ithread,icb);fflush(stdout);

    if(dabufpi == NULL)
    {
      printf("[%d] OUT: ERROR in sro_get() - exit\n",ithread);
      goto exit;
    }
    len = dabufpi[0]*4; /* buffer length in bytes */
    //printf("[%d] OUT: got dabufpi[], len=%d\n",ithread,len);fflush(stdout);

    //printf("11\n");fflush(stdout);
    //buffercheck1(dabufpi,1);
    //printf("12\n");fflush(stdout);

#ifndef DUMMY_OUT_OUTPUT
    /* send output buffer length (bytes) */
    res = send(connection, (unsigned int *)&len, sizeof(len), flags);
    //printf("[%d] OUT: send len=%d, returns res=%d\n",ithread,len,res);
    if(res<0)
    {
      perror("send(): ");
      printf("[%d] OUT: send lentot returns res=%d - exits sro_output_thread\n",ithread,res);
      goto exit;
    }

    /* send output buffer */
    res = send(connection, (unsigned int *)dabufpi, len, flags);
    //printf("[%d] OUT: send len=%d, returns res=%d\n",ithread,len,res);
    if(res<0)
    {
      perror("send(): ");
      printf("[%d] OUT: send lentot returns res=%d - exits sro_output_thread\n",ithread,res);
      goto exit;
    }
#endif

  } /*while(theOut->exit==0)*/





exit:
  theOut->res = res;
  close(connection);
  theOut->exit = -1;
  //close(output_socket);
  printf("[%d] OUT: calling pthread_exit\n",ithread);fflush(stdout);
  pthread_exit(0);
}







/************************/
/*** memory available ***/



#include <sys/sysinfo.h>
           
static unsigned long mem_avail()
{
  struct sysinfo info;
  
  if (sysinfo(&info) < 0)
    return 0;
    
  return(info.freeram);
}







/**************/
/* CODA stuff */
/**************/

int
sroConstructor()
{
  localobject->privated = (void *) &ETP;
  bzero ((char *) &ETP,sizeof(ETP));

  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;

  int ii ,jj, kk, id, res, icb, status;
  uint64_t maxAvailBytes, maxNeededBytes, memfactor;
  pthread_attr_t attr;

  unsigned int *dabufp;

  printf("sroConstructor reached\n");fflush(stdout);

  etp->nthreads = 0;
  etp->exit = 0;

  /*
  for(i=0; i<MAXLINKS; i++)
  {
    printf("[%d] etp->links_id[%d] = 0x%08x\n",i,i,etp->links_id[i]);
  }
  printf("[%d] etp->fbs_id = 0x%08x\n",i,etp->fbs_id);
  printf("etp->exit = %d\n",etp->exit);
  */

  printf("sroConstructor 1\n");fflush(stdout);


  printf("Available memory = %lu MByte\n",mem_avail()/1024/1024);
  maxAvailBytes = mem_avail()/2; //never use more then 50% of available memory

  send_buf_size_bytes = SEND_BUF_SIZE;
  printf("initial send_buf_size_bytes=%lu MByte\n",send_buf_size_bytes/1024/1024);
  maxNeededBytes = MAXLINKS*NUM_SEND_BUFS*send_buf_size_bytes + MAXOUTS*NUM_SEND_BUFS*send_buf_size_bytes*2;
  memfactor = maxNeededBytes / maxAvailBytes;
  printf("%u %u %u + %u %u %u\n",MAXLINKS,NUM_SEND_BUFS,send_buf_size_bytes,MAXOUTS,NUM_SEND_BUFS,send_buf_size_bytes*4);
  printf("maxNeededBytes=%lu, maxAvailBytes=%lu -> memfactor=%lu\n",maxNeededBytes,maxAvailBytes,memfactor);
  send_buf_size_bytes = SEND_BUF_SIZE/(memfactor+1);
  printf("new send_buf_size_bytes=%lu MByte\n",send_buf_size_bytes/1024/1024);


  /***************************/
  /* allocate output buffers */

  for(ii=0; ii<MAXLINKS; ii++)
  {
    /* between input threads and fb thread */

    gbigBUFIN[ii] = NULL;
    gbigBUFIN[ii] = sro_new(ii, NUM_SEND_BUFS, send_buf_size_bytes);
    if(gbigBUFIN[ii] == NULL)
    {
      printf("ERROR in sro_new: gbigBUFIN[%d] allocation FAILED\n",ii);
      exit(1);
    }
    else
    {
      printf("sro_new: gbigBUFIN[%d] allocated %lld Mbytes at 0x%08x\n",ii,NUM_SEND_BUFS*(send_buf_size_bytes/1024/1024),gbigBUFIN[ii]);
    }
    if(gbigBUFIN[ii] != NULL)
    {
      sro_init(&gbigBUFIN[ii]);
      dabufp = sro_write_current(&gbigBUFIN[ii], &icb); /* dabufp is not used here, just to check if buffer was prepared */
      if(dabufp == NULL)
      {
        printf("ERROR in sro_write_current: FAILED\n");
        exit(1);
      }
    }
  }

  printf("sroConstructor 2 (MAXOUTS=%d)\n",MAXOUTS);fflush(stdout);

  for(ii=0; ii<MAXOUTS; ii++)
  {
    printf("sroConstructor 21 (MAXOUTS=%d, ii=%d)\n",MAXOUTS,ii);fflush(stdout);

    /* between fb thread and output thread */
    gbigBUFOUT[ii] = NULL;
    gbigBUFOUT[ii] = sro_new(ii, NUM_SEND_BUFS, (send_buf_size_bytes*2));
    if(gbigBUFOUT[ii] == NULL)
    {
      printf("ERROR in sro_new: gbigBUFOUT[%d] allocation FAILED\n",ii);fflush(stdout);
      exit(1);
    }
    else
    {
      printf("sroConstructor/sro_new: gbigBUFOUT[%d] allocated %lld Mbytes at 0x%08x\n",ii,NUM_SEND_BUFS*(send_buf_size_bytes/1024/1024)*2,gbigBUFOUT[ii]);fflush(stdout);
    }
    if(gbigBUFOUT[ii] != NULL)
    {
      sro_init(&gbigBUFOUT[ii]);
      dabufp = sro_write_current(&gbigBUFOUT[ii], &icb); /* dabufp is not used here, just to check if buffer was prepared */
      if(dabufp == NULL)
      {
        printf("ERROR in sro_write_current: FAILED\n");
        exit(1);
      }
    }
  }
  
  printf("sroConstructor 3\n");fflush(stdout);

  /***************************/
  /***************************/

  tcpState = DA_BOOTED;
  if(codaUpdateStatus("booted") != ET_OK) return(ET_ERROR);

  printf("sroConstructor: calling tcpServer(%s, %s)\n",localobject->name, mysql_host);
  tcpServer(localobject->name, mysql_host); /*start server to process non-coda commands sent by tcpClient*/

  return(ET_OK);
}


int
sroDestructor()
{
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;
}


/*********************************************************/
/*********************************************************/
/*********************************************************/


int
codaInit(char *conf)
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



static void
distribute_bits(uint64_t global_mask, int num_targets, uint64_t *bitmasks, int *bitnums)
{
  // Reset all target variables
  for (int i = 0; i < num_targets; i++)
  {
    bitmasks[i] = 0;
    bitnums[i] = 0;
  }
  
  int current_target = 0;
    
  // Loop while there are still bits set in the global_mask
  while (global_mask != 0)
  {
    // Isolate the lowest set bit
    uint64_t lowest_bit = global_mask & -global_mask;
        
    // Add the bit to the current target variable
    bitmasks[current_target] |= lowest_bit;
    bitnums[current_target] ++;
    
    // Move to the next target in a round-robin way
    current_target = (current_target + 1) % num_targets;
        
    // Clear the lowest set bit from the global mask
    global_mask &= global_mask - 1;
  }
}


int
codaDownload(char *conf)
{
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;
  LINKS theLink;
  FBS theFb;
  OUT theOut;
  int have_all_inputs = 0;
  FILE *fd;
  int roc_id, slot, nslots, board_type;
  unsigned int slotMask = 0;
  unsigned int fadcSlotMask = 0;
  unsigned int dcrbSlotMask = 0;
  char filein[256], host_no_vtp[80];
  hrtime_t ttt;
  uint64_t targets[MAXFBS];
  int      itargets[MAXFBS];

  static char tmp[1000];
  static char tmp1[1000];
  static char tmp2[1000];
  static char tmp3[1000];
  int  ii, jj, i, ix, status, ntransferred;  
  char *ch, *p_sl;
  int  listArgc;
  char listArgv[LISTARGV1][LISTARGV2];

  MYSQL *dbsock;
  char tmpp[1000];

  int res, len;
  pthread_attr_t attr;

  printf("000\n");fflush(stdout);
  printf("000 conf[0]..=>%c%c%c%c..<\n",conf[0],conf[1],conf[2],conf[3]);fflush(stdout);
  printf("000 conf=>%s<\n",conf);fflush(stdout);

  etp->object = object;
  /*
  printf("111 configname=0x%08x\n",configname);fflush(stdout);
  configname[0] = 'a';
  configname[10] = 'b';
  printf("111 configname[0]=%c\n",configname[0]);fflush(stdout);
  printf("111 configname[10]=%c\n",configname[10]);fflush(stdout);
  */
  /*****************************/
  /*****************************/
  /* FROM CLASS (former conf1) */

  strcpy(configname,conf); /* save CODA configuration name */
  printf("112\n");fflush(stdout);
  printf("coda_sro: configname = >%s<\n",configname);fflush(stdout);


  UDP_start();

  tcpState = DA_DOWNLOADING;
  if(codaUpdateStatus("downloading") != ET_OK) return(ET_ERROR);



  /*****************************/
  /*****************************/
  /*****************************/
  printf("222\n");fflush(stdout);


  /* use 'standalone' configuration name to get info from database, original configuration name will be restored after that */
  if(standalone)
  {
    strcpy(configname,configalone);
    printf("Use STANDALONE configuration >%s< TEMPORARY\n",configname);
  }


  /***************************************************/
  /* extract all necessary information from database */

  /* connect to database */
  dbsock = dbConnect(mysql_host, expid);
  printf("333\n");fflush(stdout);




  /*obtain and decode database 'code' field */
  sprintf(tmp,"SELECT code FROM %s WHERE name='%s'",configname,object->name);
  printf("MYSQL QUERY >%s<\n",tmp);
  if(dbGetStr(dbsock, tmp, tmpp)==ET_ERROR)
  {
    printf("coda_sro: ERROR in mysql query >%s<\n",tmp);
  }
  else
  {
    printf("coda_sro: code >%s< len=%d\n",tmpp,strlen(tmpp));
  }


  
  /************************/
  /* decode 'code' string */
  /************************/
  strcpy(tmp, tmpp);
  if(!((strcmp (tmp, "{}") == 0)||(strcmp (tmp, "") == 0)))
  {
    if(listSplit1(tmp, 1, &listArgc, listArgv))
    {
      return(CODA_ERROR);
    }

    /* the number of {} cannot exceed 2 */
    if(listArgc>2)
    {
      printf("ERROR: listArgc=%d, set it to 2\n",listArgc);
      listArgc = 2;
    }

    for(ix=0; ix<listArgc; ix++) printf("listArgc [%1d] >%s<\n",ix,listArgv[ix]);

    if(listArgc<=0)
    {
      printf("ERROR: no code(s) - exit\n");
      exit(0);
    }

    /* listArgv[0] suppose to contains the list of input hosts */
    strcpy(tmp1,listArgv[0]);

    /* listArgv[1] contains output file directory name */
    strcpy(tmp2,listArgv[1]);

    /* listArgv[2] contains SRO component type: SRO(=none-default), TRIDAS, .. */
    strcpy(tmp3,listArgv[2]);

    printf("INFO: tmp1 >%s<, tmp2 >%s<, tmp3 >%s<\n",tmp1,tmp2,tmp3);

    printf("code parsed\n");
  }
  else
  {
    printf("WARN: no code's\n");
  }




#if 1
  
  /*************************************************************************************************/
  /* AT THAT POINT, tmp1 contains input list; it is obsolete, get one from database 'inputs' field */


  /*obtain and decode database 'inputs' field */
  sprintf(tmp,"SELECT inputs FROM %s WHERE name='%s'",configname,object->name);
  printf("MYSQL QUERY >%s<\n",tmp);
  if(dbGetStr(dbsock, tmp, tmpp)==ET_ERROR)
  {
    printf("coda_sro: ERROR in mysql query >%s<\n",tmp);
  }
  else
  {
    printf("coda_sro: inputs >%s< len=%d\n",tmpp,strlen(tmpp));
  }

  /* convert
        test0vtp:test0vtp test2vtp:test2vtp
     into
        test0vtp-s1:test0vtp-s1 test0vtp-s2:test0vtp-s2 test2vtp-s1:test2vtp-s1 */


  listArgc = 0;
  if( !( (strcmp(tmpp, "") == 0) || (strcmp(tmpp, "none") == 0) || (strlen(tmpp) < 3) ) )
  {
    if(listSplit1(tmpp, 0, &listArgc, listArgv)) return(CODA_ERROR);
    for(ix=0; ix<listArgc; ix++)
    {
      printf("input [%1d] >%s<\n",ix,listArgv[ix]);
    }

    ix = 0;
    for(ii=0; ii<listArgc; ii++)
    {
      ch = strchr(listArgv[ii],':'); /* get pointer to the location of ':' */
      if(ch==NULL)
      {
        printf("wrong arguments in 'inputs' - exit\n");
        exit(0);
      }
      ch[0] = '\0'; /* replace ':' with end-of-string */
      ch ++; /* put pointer to the next position after ':' */

      /* from input 'test0vtp:test0vtp', create two actual host names: 'test0vtp-s1' and 'test0vtp-s2' (ALWAYS ASSUMING TWO STREAMS FROM EVERY VTP) */
      strcpy(etp->name[ix],listArgv[ii]);
      strcat(etp->name[ix],"-s1");
      strcpy(etp->host[ix],ch);
      strcat(etp->host[ix],"-s1");
      printf("INPUT LIST: name[%d] >%s<, host[%d] >%s<\n",ix,etp->name[ix],ix,etp->host[ix]);
      ix ++;


      
      /*************************************************************************************************************/
      /*if that crate has only one FADC, do not open second link (make it subroutine ? used in vtprol1 as well ...)*/
      len = strlen(listArgv[ii]);
      strncpy(host_no_vtp,listArgv[ii],len-3);
      host_no_vtp[len-3] = '\0';
      printf("host_no_vtp >%s<\n",host_no_vtp);
      sprintf(filein,"%s/sro/%s.%s",getenv("CLON_PARMS"),host_no_vtp,"txt");
      if((fd=fopen(filein,"r")) == NULL)
      {
        printf("Cannot open input file >%s< - exit\n",filein);
        return(CODA_ERROR);
      }
      else
      {
        printf("Opened input file >%s< for reading\n",filein);
      }


      while( fscanf(fd,"%d %x %x",&roc_id,&slotMask,&board_type) != EOF )
      {
        printf("roc_id=%d, slotMask is 0x%08x, board type is 0x%01x\n",roc_id,slotMask,board_type);
        if(board_type==0x1)
        {
          fadcSlotMask = slotMask;
          if(fadcSlotMask>0) printf("==> roc_id=%d, fadcSlotMask is 0x%08x, board type is 0x%01x\n",roc_id,fadcSlotMask,board_type);
        }
        else if(board_type==0x2)
        {
          dcrbSlotMask = slotMask;
          if(dcrbSlotMask>0) printf("==> roc_id=%d, dcrbSlotMask is 0x%08x, board type is 0x%01x\n",roc_id,dcrbSlotMask,board_type);
        }
        else
        {
          printf("==> ERROR: unknown board type 0x%01x, or unspecified one - exit\n",board_type);
          exit(1);
        }
      }
      printf("\n");

      fclose(fd);


      /*check if we have anything*/
      slotMask = fadcSlotMask | dcrbSlotMask;
      if(slotMask==0)
      {
        printf("ERROR: there are no known board types in the crate - exit\n");
        exit(1);
      }
      else
      {
        printf("\n===== will use slotMask=0x%08x =====\n\n",slotMask);
      }

      
      nslots = 0;
      for(slot=0; slot<21; slot++)
      {
        if( ((slotMask>>slot)&0x1)==1 )
        {
          nslots ++;
        }
      }
      printf("\n>> Nslots = %d\n\n",nslots);
      /*************************************************************************************************************/
      /*************************************************************************************************************/


      if(nslots>1)
      {
        strcpy(etp->name[ix],listArgv[ii]);
        strcat(etp->name[ix],"-s2");
        strcpy(etp->host[ix],ch);
        strcat(etp->host[ix],"-s2");
        printf("INPUT LIST: name[%d] >%s<, host[%d] >%s<\n",ix,etp->name[ix],ix,etp->host[ix]);
        ix ++;
      }
      
      /* delete from database all names alike our name before '-' */
      printf("ch(befor) >%s<\n",ch);
      for(i=0; i<strlen(ch); i++)
      {
        if(ch[i]=='-') {ch[i]='\0'; break;}
      }
      printf("ch(after) >%s<\n",ch);
    }
    
    if(ix<=MAXLINKS)
    {
      etp->nthreads = ix;
    }
    else
    {
      printf("INCREASE 'MAXLINKS' TO AT LEAST %d !!!\n",ix);
      exit(0);
    }
    printf("\n>>> Will connect to following %d VTP links:\n",etp->nthreads);
    for(ii=0; ii<etp->nthreads; ii++)
    {
      printf("      thread[%d]: name >%s<, host >%s<\n",ii,etp->name[ii],etp->host[ii]);
    }
    printf("\n");
  }

  
  /*************************************************************************************************/
  /*************************************************************************************************/

#endif

  
  /* parse outputs list */
  listArgc = 0;
  if(!((strcmp(tmp2, "") == 0)||(strlen(tmp2) < 3)))
  {
    if(listSplit1(tmp2, 0, &listArgc, listArgv)) return(CODA_ERROR);
    for(ix=0; ix<listArgc; ix++)
    {
      printf("input2 [%1d] >%s<\n",ix,listArgv[ix]);
    }

    /* search for anything started from '/' */
    for(ix=0; ix<listArgc; ix++)
    {
      ch = strchr(listArgv[ix],'/'); /* get pointer to the location of '/' */
      if(ch!=NULL)
      {
        etp->output_file = 1;
        strcpy(tmp1,listArgv[ix]);
        printf("Obtained string >%s<, forming directory and file names\n",tmp1);

	/* extract string after last slash */
        p_sl = strrchr(tmp1, '/');
        strcpy(etp->subfilename,p_sl+1);
        strcpy(etp->subdirname,tmp1);
        printf("will use subdirectory >%s<, subfile >%s<\n\n",etp->subdirname,etp->subfilename);
      }
      else
      {
        etp->output_file = 0;
      }

      //if(!strncmp(listArgv[ix],"stream",6))
	  //{
      //  printf("Will open output sockets\n");
      //  output_sockets = 1;
	  //}
    }

  }


  /* parse type */
  listArgc = 0;
  if(!((strcmp(tmp3, "") == 0)||(strlen(tmp3) < 3)))
  {
    if(listSplit1(tmp3, 0, &listArgc, listArgv)) return(CODA_ERROR);
    for(ix=0; ix<listArgc; ix++)
    {
      printf("input3 [%1d] >%s<\n",ix,listArgv[ix]);
    }

    /*  */
    for(ix=0; ix<listArgc; ix++)
    {
      if(!strncmp(listArgv[ix],"TRIDAS",6))
      {
        printf("Type is TRIDAS\n");
        //logic_type = 1;
      }
      else
      {
        //logic_type = 0;
      }
    }

  }



  /* disconnect from database */
  dbDisconnect(dbsock);



  /* restore original configuration name */
  if(standalone)
  {
    strcpy(configname,conf);
    printf("Restore configuration >%s<\n",configname);
  }


  /*****************************/
  /*****************************/
  /*****************************/

  etp->exit = 0;


  printf("INFO: Downloading configuration '%s'\n", configname);

  /* ???
  printf("etp->nthreads = %d\n",etp->nthreads);
  for(i=0; i<etp->nthreads; i++)
  {
    printf("[%d] etp->links_id[%d] = 0x%08x\n",i,i,etp->links_id[i]);
  }
  printf("etp->fbs_id = 0x%08x\n",etp->fbs_id);
  printf("etp->exit = %d\n",etp->exit);
  */
  
  /* if not the first entry, exit sro_input_thread 
  if(etp->id != 0)
  {
    etp->exit = 1;
    while(etp->exit == 1)
    {
      printf("Waiting for sro_input_thread thread to exit ...\n");
      sleep(1);
    }
    printf("sro_input_thread to exited !\n");
    sleep(10);
  }
  */

  /*
  if(etp->id != 0)
  {
    printf("Sending kill request to sro_input_thread ...\n");
    pthread_kill(etp->id, 0);
  }
  sleep(10);
  */


  for(ii=0; ii<etp->nthreads; ii++) etp->links_id[ii] = 0;
  for(ii=0; ii<MAXFBS; ii++) etp->fbs_id[ii] = 0;
  for(ii=0; ii<MAXOUTS; ii++) etp->outs_id[ii] = 0;


  /************************/
  /* create input threads */

  dtimeout = 1000; /*100000*/ /*microsec*/
  printf("\netp->nthreads = %d\n\n",etp->nthreads);
  mask_needed_global = 0ULL; /*non-zero bits corresponds to active streams*/
  for(i=0; i<etp->nthreads; i++)
  {
    if(gbigBUFIN[i] != NULL)
    {
      sro_init(&gbigBUFIN[i]);
    }

    /*set appropriate bit in bitmask*/
    mask_needed_global |= (1ULL << i);

    /* if the first entry, start pthread(s) */
    if(etp->links_id[i] == 0)
    {
      struct sched_param param;

      /********************************************************/
      /* allocate memory for structure 'theLink' and fill it; */
      /* will be free'd in .......                            */
      /********************************************************/
      theLink = (LINKS) calloc(sizeof(LINK_S),1);
      etp->links[i] = theLink;

      printf("Starting input thread [%d]: name >%s<, host >%s<\n",i,etp->name[i],etp->host[i]);fflush(stdout);
      theLink->exit = 0;
      theLink->database_updated = 0;
      theLink->dtime_updated = 0;
      theLink->ithread = i;
      strcpy(theLink->name, etp->name[i]);
      strcpy(theLink->host, etp->host[i]);
      theLink->outbuf = gbigBUFIN[i];
      theLink->fd_in = 0; /* set it to zero here, will check it in Prestart to see if link from VTP is accepted */

      pthread_attr_init(&attr); /* initialize attr with default attributes */
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
      pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

      pthread_attr_getschedparam(&attr,&param);
      printf("[%d] IN: thread default priority is %d\n",i,param.sched_priority);
      param.sched_priority = 30;
      pthread_attr_setschedparam(&attr,&param);
      pthread_attr_getschedparam(&attr,&param);
      printf("[%d] IN: thread new priority is %d\n",i,param.sched_priority);

      res = pthread_create(&etp->links_id[i], &attr, (void *(*)(void *)) sro_input_thread, (void *) theLink);
      if(res!=0)
      {
        printf("ERROR: pthread_create(sro_input_thread) returned %d - exit\n",res);
        exit(-1);
      }
      else
      {
        printf("sro_input_thread started, etp->links_id[%d] = 0x%08x\n",i,etp->links_id[i]);
        theLink->thread = etp->links_id[i];
      }
      
      while(theLink->database_updated==0)
      {
        printf("Waiting 500 milliseconds for input thread %d to update database\n",i);fflush(stdout);
        usleep(500000);
        //printf("Waiting 1 second for input thread %d to update database\n",i);fflush(stdout);
        //sleep(1);
      }

    } /*check if first entry*/

  } /*loop over input threads*/


  printf("== mask_needed_global=0x%llx\n",mask_needed_global);
  

  /*************************/
  /* create output threads */
  for(ii=0; ii<MAXOUTS; ii++)
  {
    /*done already ???
    if(gbigBUFOUT[ii] != NULL)
    {
      sro_init(&gbigBUFOUT[ii]);
    }
    */

    if(etp->outs_id[ii] == 0)
    {
      theOut = (OUT) calloc(sizeof(OUT_S),1);
      etp->outs[ii] = theOut;

      theOut->exit = 0;
      theOut->ithread = ii;
      theOut->inbuf = gbigBUFOUT[ii]; /*our input bugbufs*/

      pthread_attr_init(&attr); /* initialize attr with default attributes */
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
      pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
      res = pthread_create(&etp->outs_id[ii], &attr, (void *(*)(void *)) sro_output_thread, (void *) theOut);
      if(res!=0)
      {
        printf(" sro_output_thread: ERROR: pthread_create(sro_output_thread) returned %d - exit\n",res);
        exit(-1);
      }
      else
      {
        //printf("[%d] sro_output_thread started, etp->outs_id[%d] = 0x%08x\n",ithread,ii,etp->outs_id[ii]);
        theOut->thread = etp->outs_id[ii];
      }
    }
  }


  


  /********************************/
  /* create frame builder threads */

  printf("\n\nStarting FB thread ..\n");fflush(stdout);


  etp->nfbs = etp->nthreads;
  if(etp->nfbs<=0)            etp->nfbs = 1;
  else if(etp->nfbs > MAXFBS) etp->nfbs = MAXFBS;
  printf("\nFB: etp->nfbs=%d\n\n",etp->nfbs);

  aggr_len = AGGR_HEADER_LENGTH + etp->nthreads;
  
  distribute_bits(mask_needed_global, etp->nfbs, targets, itargets);
  for(ii=0; ii<etp->nfbs; ii++) printf("targets[%d] = 0x%llx, itargets[%d]=%d\n",ii,targets[ii],ii,itargets[ii]);

  for(ii=0; ii<etp->nfbs; ii++)
  {
    if(etp->fbs_id[ii] == 0)
    {
      theFb = (FBS) calloc(sizeof(FB_S),1);
      etp->fbs[ii] = theFb;

      theFb->mask_needed = targets[ii];
      theFb->mask_num_bits = itargets[ii];
      printf("===> mask_needed for FB[%d] is 0x%08x, nbits(nstreams)=%d (global=0x%08x)\n",ii,theFb->mask_needed,theFb->mask_num_bits,mask_needed_global);
      
      theFb->exit = 0;
      theFb->ithread = ii;
      for(i=0; i<etp->nthreads; i++) theFb->inbuf[i] = gbigBUFIN[i]; /*input bigbufs*/    
      for(i=0; i<MAXOUTS; i++)
      {
	/*output bigbufs*/
	theFb->outbuf[i] = gbigBUFOUT[i];

	/*starting output buffers pointers*/
        dabufpo[i] = sro_write_current(&(theFb->outbuf[i]), &icbout[i]);
        if(dabufpo[i] == NULL)
        {
          printf("FB ERROR in sro_write_current: FAILED\n");
          exit(1);
        }
        else
        {
	  next_index[i] = 1 + aggr_len; /*reserve space for aggregated header*/

          printf("From 0x%llx, grabbed first circular buffer for FB output %d: dabufpo=0x%llx, icb=%d\n",theFb->outbuf[i],i,dabufpo[i],icbout[i]);fflush(stdout);
          dabufpo[i][0] = 1 + aggr_len; /* inclusive length (itself + aggr_len) */
	  dabufpo[i][9] = (0x42<<24) | (0x1<<16) | etp->nthreads; /* fill 8rd word in aggregated header, will be used by coda_spr.c */
        }
      }

      ttt = gethrtime();
      for(i=0; i<MAXOUTS; i++) fbtimeout[i] = ttt + 1*100000;



      
      pthread_attr_init(&attr); /* initialize attr with default attributes */
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
      pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
      res = pthread_create(&etp->fbs_id[ii], &attr, (void *(*)(void *)) sro_fb_thread, (void *) theFb);
      if(res!=0)
      {
        printf("ERROR: pthread_create(sro_fb_thread) returned %d - exit\n",res);
        exit(-1);
      }
      else
      {
        printf("[%d] sro_fb_thread started, etp->fbs_id[%d] = 0x%08x\n",ii,ii,etp->fbs_id[ii]);
        theFb->thread = etp->fbs_id[ii];
      }
    }
  }
  


  /* wait for sro_output_threads to fill 'port_out' */
  printf("checking sro_fb_thread's port_out's ..\n");
  for(ii=0; ii<MAXOUTS; ii++)
  {

    fbstr[ii].frame_num = -1;
    fbstr[ii].inputs_mask = 0ULL;
    
    while(etp->outs[ii]->port_out==0)
    {
      printf("Waiting for sro_output_thread[%d] port_out ...\n",ii);fflush(stdout);
      sleep(1);
    }
    printf("Got it: sro_output_thread[%d] port_out=%d\n",ii,etp->outs[ii]->port_out);fflush(stdout);
  }











  





  
  /* we assume that database update by previous section is finished */

  printf("coda_sro: downloaded !!!\n");

  tcpState = DA_DOWNLOADED;
  if(codaUpdateStatus("downloaded") != ET_OK) return(ET_ERROR);

  return(ET_OK);
}


int
codaPrestart()
{
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;

  MYSQL *dbsock;
  char subdirname[128], tmpp[1000];
  int i, j, ix, ii, jj;
  int have_all_inputs = 0;
  int ports[MAXLINKS];
  mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;

  int res;
  pthread_attr_t attr;

  /* connect to database */
  dbsock = dbConnect(mysql_host, expid);

  /* Get the run number */
  sprintf(tmpp,"SELECT runNumber FROM sessions WHERE name='%s'",session);
  if(dbGetInt(dbsock,tmpp,&(object->runNumber))==CODA_ERROR) return(CODA_ERROR);

  /* DO WE NEED TO GET runType AS WELL ??? */

  /* disconnect from database */
  dbDisconnect(dbsock);

  printf("INFO: prestarting, run %d, type %d\n",
    object->runNumber, object->runType);

  object->nevents = 0;
  object->nlongs = 0;

  for(i=0; i<etp->nthreads; i++)
  {
    printf("Expect connection FROM=%d\n",etp->links[i]->port_in);
  }

  /* wait for all VTPs to connect to our threads */
  printf("\n");
  while(have_all_inputs == 0)
  {
    have_all_inputs = 1;
    for(i=0; i<etp->nthreads; i++)
    {
      if(etp->links[i]->fd_in == 0)
      {
        printf("Waiting for input thread [%d] to accept connection from VTP >%s< ..\n",i,etp->links[i]->name);
        have_all_inputs = 0;
      }
      else
      {
        printf("Established input thread [%d] connection from VTP >%s< !\n",i,etp->links[i]->name);
      }
    }
    printf("\n");
    
    sleep(1);

    //if(etp->exit == 1)
    //{
    //  printf("Did not get all connections from VTPs, but received 'Exit' command - returning from Prestart\n");
    //  return(ET_ERROR);
    //}
  }
  printf("Got all VTPs connected\n");



  /* gracefully stop sro loop */
  sro_loop_exit = 1;
  ii = 3;
  while(sro_loop_exit)
  {
    printf("download: wait for sro_loop to exit ..\n");
    sleep(1);
    ii --;
    if(ii<0) break;
  }

  if(ii<0)
  {
    printf("WARN: cannot exit sro_loop gracefully, will kill it\n");
    /* TODO: delete sro_loop thread */
    sleep(1);

    sro_loop_exit = 0; /* to let new sro_loop to start */
  }


  /* Spawn the sro_loop Thread */
  {
    pthread_attr_init(&detached_attr);
    pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(&detached_attr,PTHREAD_SCOPE_SYSTEM/*PTHREAD_SCOPE_PROCESS*/);
    pthread_create( /*(unsigned int *)*/ &iTaskROL, &detached_attr,
		   (void *(*)(void *)) sro_loop, (void *) NULL);
  }



  tcpState = DA_PAUSED;
  codaUpdateStatus("paused");

  return(ET_OK);
}

int
codaGo()
{
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;
  int have_all_outputs = 0;
  int i, jj;

  tcpState = DA_ACTIVE;
  codaUpdateStatus("active");

  

  /*dummy backend - works, but cannot connect to vtp second time !!!
  printf("Exiting input threads !!!!!!!\n");fflush(stdout);
  printf("Exiting input threads !!!!!!!\n");fflush(stdout);
  printf("Exiting input threads !!!!!!!\n");fflush(stdout);
  printf("Exiting input threads !!!!!!!\n");fflush(stdout);
  printf("Exiting input threads !!!!!!!\n");fflush(stdout);
  for(i=0; i<etp->nthreads; i++)
  {
    etp->links[i]->exit = 1;
  }
  sleep(10);
  */
  
  return(ET_OK);
}

int
codaEnd()
{
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;
  int ii, jj;


  printf("Sending exit command to FB threads ..\n");fflush(stdout);
  for(ii=0; ii<etp->nfbs; ii++)
  {
    if(etp->fbs_id[ii] != 0)
    {
      printf("Requesting FB[%d] to exit\n",ii);
      etp->fbs[ii]->exit = 1;      
    }
  }

  sleep(3);

  printf("Checking FB threads ..\n");fflush(stdout);
  for(ii=0; ii<etp->nfbs; ii++)
  {
    if(etp->fbs[ii]->exit!=-1)
    {
      pthread_cancel(etp->fbs_id[ii]);
      printf("FB thread canceled\n");
    }
    else
    {
      printf("FB thread already exited\n");
    }
  }


  
  tcpState = DA_DOWNLOADED;
  codaUpdateStatus("downloaded");
  return(ET_OK);  
}

int
codaPause()
{
  tcpState = DA_PAUSED;
  codaUpdateStatus("paused");
  return(ET_OK);
}

int
codaExit()
{
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;
  int ii, jj;

  printf("Exiting ..\n");fflush(stdout);

#if 0
  sleep(1);
  for(ii=0; ii<etp->nthreads; ii++)
  {
    etp->links[ii]->exit = 1;
    printf("[%d] Sent exit command to input thread ..\n",ii);fflush(stdout);
    sleep(3);
    printf("Checking input thread ..\n");fflush(stdout);
    if(etp->links[ii]->exit!=-1)
    {
      //pthread_cancel(etp->links_id[ii]);
      printf("input thread canceled\n");
    }
    else
    {
      printf("input thread already exited\n");
    }
  }
  sleep(3);



  etp->fbs->exit = 1;
  printf("Sent exit command to FB thread ..\n");fflush(stdout);
  sleep(1);
  printf("Checking FB thread ..\n");fflush(stdout);
  if(etp->fbs->exit!=-1)
  {
    //pthread_cancel(etp->fbs_id);
    printf("FB thread canceled\n");
  }
  else
  {
    printf("FB thread already exited\n");
  }
#endif



  
  sleep(1);
  printf("Exiting codaExit ..\n");
  etp->exit = 1;

  tcpState = DA_CONFIGURED;
  codaUpdateStatus("configured");
  /*
  UDP_reset();
  */
  return(ET_OK);
}


void
coda_sro(char *conf)
{
  codaInit(conf);
  codaDownload(conf);
  codaPrestart();
  codaGo();
  while(1)
  {
    sleep(1);
	/*
    printf("STA: events=%12.3f event_rate=%12.3f data=%12.3f data_rate=%12.3f\n",
		   statistics[0],statistics[1],statistics[2],statistics[3]);
	*/
  }


}

/****************/
/* main program */
/****************/


void
main (int argc, char **argv)
{
  printf("111\n");fflush(stdout);

  CODA_Init(argc, argv);

  printf("222\n");fflush(stdout);

  sroConstructor();

  printf("333\n");fflush(stdout);

  if(standalone)
  {
    printf("444\n");fflush(stdout);
    coda_sro("FT1_TRIDAS");
    exit;
  }

  printf("\n\n\n sroConstructor done, running CODA_Execute\n\n");fflush(stdout);
  /* CODA_Service ("SRO"); */
  CODA_Execute ();

  sroDestructor(); /* never called ... */
}


#endif /*if defined(Linux_armv7l)*/
