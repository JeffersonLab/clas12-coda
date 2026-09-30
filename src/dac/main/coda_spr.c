
/* coda_spr.c */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//#define DEBUG

//#define DUMMY_SPR

#if defined(Linux_armv7l)

int
main()
{
  printf("coda_spr is dummy for ARM etc\n");
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
#include <sched.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <pthread.h>
#include <limits.h>
#include <sys/param.h>


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

//evio
#include "evio.h"
#include "evioBankUtil.h"
//evio




/* big buffers */
static int *BUFIN[MAXOUTS];
static int *BUFOUT[MAXOUTS];

static int spr_loop_exit = 0;
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
spr_loop()
{
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;
  int ii, jj;
  int32_t nevents;
  int64_t nlongs;
  int livetime;
  int64_t nlongs_old[MAXOUTS], nlongs_rate[MAXOUTS], nlongs_rate_total;

#ifdef Linux
  prctl(PR_SET_NAME,"spr_loop");
#endif
  printf("spr_loop started\n");

  /* some initialization */

  for(ii=0; ii<MAXOUTS; ii++) nlongs_old[ii] = 0LL;
  sroStatPrint(1); //clean statistics

  while(1)
  {

    if(spr_loop_exit)
    {
      spr_loop_exit = 0;
      return;
    }
    /*printf("spr_loop...\n");*/

    /* nevents and nlongs are accumulating inside every thread, here we just summing them */
    nevents = 0;
    nlongs = 0LL;
    livetime = 0;
    nlongs_rate_total = 0LL;

    for(ii=0; ii<MAXOUTS; ii++)
    {
      nevents += etp->procs[ii]->nevents;
 
      nlongs += etp->procs[ii]->nlongs;
      livetime += etp->procs[ii]->livetime;

      nlongs_rate[ii] = ((etp->procs[ii]->nlongs - nlongs_old[ii])*4LL) / 1024LL;
      //#ifdef PRINT_STAT
      if(tcpState == DA_ACTIVE) printf(" [%d] Data rate = %6lld kB/s, livetime = %3d %\n",ii,nlongs_rate[ii],etp->procs[ii]->livetime);
      //#endif
      nlongs_rate[ii] = nlongs_rate[ii] / 1024LL;

      nlongs_rate_total += nlongs_rate[ii];
      nlongs_old[ii] = etp->procs[ii]->nlongs;
    }
    object->nevents = nevents;
    object->nlongs = nlongs;
#ifdef PRINT_STAT
    if(etp->nthreads>0) printf("Total rate = %6lld MB/s, average livetime = %3d %\n",nlongs_rate_total,livetime/etp->nthreads);
    printf("\n");
#endif
    if(tcpState == DA_ACTIVE)
    {
      sroStatPrint(1);
      printf("\n");
    }
    
    sleep(1);
  }

  return;
}



int
hostname_to_ip(char *hostname, char *ip)
{
  int sockfd;  
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_in *h;
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
  hints.ai_socktype = SOCK_STREAM;

  if ( (rv = getaddrinfo( hostname , "http" , &hints , &servinfo)) != 0) 
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) 
  {
    h = (struct sockaddr_in *) p->ai_addr;
    strcpy(ip , inet_ntoa( h->sin_addr ) );
  }
	
  freeaddrinfo(servinfo); // all done with this structure

  return(0);
}




















/****************************************************************/
/* gets data from frame builder, process it, and output to file */

void *
spr_proc_thread(PROC theProc)
{
  int ii, jj, len, slot, res, ret, icb0, icb, slot_len, dataindex;
  int type, status, nev;
  char txt[100], name[80];
  int ithread = theProc->ithread;
  int input_socket, input_connect;
  struct sockaddr_in sin_proc;
  int port_in = 0;
  char host_in[128], ip[128];
 
  printf("---> ithread=%d\n",ithread);

  sprintf(name,"sp_%03d",ithread);
  sprintf(txt,"so_%03d",ithread); /* name of the output thread/tcp server we have to connect */
#ifdef Linux
  prctl(PR_SET_NAME,name);
#endif


  printf("\n==========================================\n");fflush(stdout);
  printf("[%d] PROC: sockets initialization =====\n\n",ithread);fflush(stdout);

  input_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(input_socket < 0)
  {
    perror("Can't open socket ");
    pthread_exit(0);
  }
  else
  {
    printf("[%d] PROC: input_socket=%d\n",ithread,input_socket);fflush(stdout);
  }

  /* obtain from database host and port of the tcp server we suppose to connect, it will be our input stream */
  codaGetStreamout(txt, host_in, &port_in);
  printf("[%d] PROC: received from database host_in=>%s<, port_in=%d\n",ithread,host_in,port_in);fflush(stdout);

  hostname_to_ip(host_in, ip);
  printf("[%d] PROC: ip >%s< (len=%d)\n",ithread,ip,strlen(ip));fflush(stdout);




  
  /*!!!!!!!!!!!!!!!!!!!!!!
To make these changes stick after a system reboot, add them to a configuration file under /etc/sysctl.d/ (or /etc/sysctl.conf): 

    Open or create a configuration file like /etc/sysctl.d/99-tcp-buffers.conf.
    Add the following lines:
    ini

    net.core.rmem_max = 16777216
    net.core.wmem_max = 16777216
    net.ipv4.tcp_rmem = 4096 87380 16777216

    Use code with caution.

Apply the changes immediately using System Configuration Utility:
bash

sudo sysctl -p /etc/sysctl.d/99-tcp-buffers.conf
!!!!!!!!!!!*/

  /*NOT SURE IF IT HELPS ...*/
  /* increase the buffer size */
  int new_buf_size = 16*1024*1024; 
  int opt_len = sizeof(new_buf_size);
  if (setsockopt(input_socket, SOL_SOCKET, SO_RCVBUF, &new_buf_size, opt_len) < 0)
  {
    perror("Failed to set receive buffer size");
  }
  int actual_rcv_size = 0;
  getsockopt(input_socket, SOL_SOCKET, SO_RCVBUF, &actual_rcv_size, (socklen_t *)&opt_len);
  printf("Updated Receive Buffer Size: %d bytes\n", actual_rcv_size);
  /*NOT SURE IF IT HELPS ...*/



  
  
  bzero(&sin_proc, sizeof(sin_proc));
  sin_proc.sin_family = AF_INET;
  sin_proc.sin_addr.s_addr = inet_addr(ip);
  sin_proc.sin_port = htons(port_in);

  if( (input_connect = connect(input_socket, (struct sockaddr *)&sin_proc, sizeof(sin_proc))) != 0 )
  {
    printf("[%d] PROC: ERROR: connection to spr_output_thread server failed, connect() returns %d, errno=%d\n",
	   ithread,input_connect,errno);fflush(stdout);
    perror("connect: "); 
    goto exit;
  }
  else
  {
    printf("[%d] PROC: INFO: connected to spr_output_thread server\n",ithread);fflush(stdout);
    theProc->connected = 1;
  }

  /* remember host and port in our structure - just in case other threads need it */
  theProc->port_in = port_in;
  strncpy(theProc->host_in, host_in, MAXSTRLEN);
  printf("[%d] PROC: INFO: host_in=>%s<, port_in=%d\n",ithread,theProc->host_in,theProc->port_in);fflush(stdout);



  /* loop until get exit flag */
  while(theProc->exit==0)
  {
    unsigned int val;
    unsigned int *buff = theProc->bufin;
    int evptr[MAXFRAME];
    //unsigned int buff[MAXBUF];

    /*read from socket*/
#ifdef DEBUG
    printf("============= PROC[%d] reading from socket\n",ithread);fflush(stdout);
#endif
    /* read the length of the expected message in bytes */
    len = read_socket(input_socket, (unsigned char *)&val, sizeof(val), 0,ithread);
    //if(len==0) goto exit; //for timed
    if(len != sizeof(val)) printf("ERROR in read_socket: return %d, must be %d\n",len,sizeof(val));
    if(val >= (MAXBUF*sizeof(int)) )
    {
      printf("ERROR in read_socket: val = %d, must be less than %d\n",val,(MAXBUF*sizeof(int)));
      exit(1); //break;
    }
#ifdef DEBUG
    printf("============= PROC[%d] befor, expect %d bytes\n",ithread,val);fflush(stdout);
#endif
    /* read message itself */
    len = read_socket(input_socket, (unsigned char *)buff, val, 0,ithread);
    //if(len==0) goto exit; //for timed
#ifdef DEBUG
    printf("============= PROC[%d] after, len=%d bytes\n",ithread,len);fflush(stdout);
#endif

    theProc->nlongs += ((len/4)+1); /*for statistics reporting only*/
    
    theProc->tot_len = buff[0]; /*will be used by sroGetSet() as total big buffer length*/


    /***********************************/
    /* process buffer and build events */

#ifdef DUMMY_SPR
    continue;
#endif

    ret = 1;
    while(ret>0) /* 'ret' will become 0 when whole 'buff' is processed, or negative in case of error */
    {
#ifdef DEBUG
      printf("[%d] Calling sroGetSet(), len=%d\n",ithread,len);
#endif
      ret = sroGetSet(buff,theProc,0,&dataindex,ithread); //third parameter turns on printing
#ifdef DEBUG
      printf("\n[%d] GETSET: ret=%d, len=%d\n",ithread,ret,buff[dataindex]);
#endif
      //for(ii=0; ii<buff[dataindex]+1; ii++) printf("=============== [%2d] 0x%08x\n",ii,buff[dataindex+ii]);

      //sroPrintSet(&buff[dataindex]);
      
      if(theProc->output_handle>0)
      {
	printf("[%d] evWrite(raw frame) calling ..\n",ithread);fflush(stdout);
        status = evWrite(theProc->output_handle,&buff[dataindex]);
        if(status!=0)
        {
          printf("PROC evWrite error, status=%d\n\n",status);
  	  exit(0);
        }
	/*
        else
        {
	  printf("evWrite(raw frame) called\n");fflush(stdout);
        }
	*/
      }
    }
    
#ifdef DEBUG
    printf("[%d] buff processed\n",ithread);
#endif



    
    if(ret>=0)
    {
        
#if 0
#ifdef DEBUG
      printf("============= PROC[%d] befor sroEventBuilder\n",ithread);fflush(stdout);
#endif

      nev = sroEventBuilder(buff, theProc->bufout, evptr);

#ifdef DEBUG
      printf("============= PROC[%d] after sroEventBuilder (nev=%d)\n",ithread,nev);fflush(stdout);
#endif
#endif /*#if 0*/

      theProc->nevents += nev;
    
    }



    
    
#ifdef DEBUG
    printf("============= PROC[%d] theProc->exit=%d\n",ithread,theProc->exit);fflush(stdout);
#endif

  } /*while(theProc->exit==0)*/






exit:

  theProc->res = res;
  close(input_socket);
  printf("[%d] spr_proc_thread: calling pthread_exit\n",ithread);fflush(stdout);
  pthread_exit(0);
}




/**************/
/* CODA stuff */
/**************/

int
sprConstructor()
{
  localobject->privated = (void *) &ETP;
  bzero ((char *) &ETP,sizeof(ETP));

  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;

  int ii, i1, i2, i3, jj, kk, id, res, icb, status;
  pthread_attr_t attr;

  unsigned int *dabufp;

  printf("sprConstructor reached\n");fflush(stdout);

  etp->nthreads = 0;
  etp->exit = 0;

  

  printf("sprConstructor 1\n");fflush(stdout);


  /********************************************/
  /* allocate buffers for evio output file(s) */

  printf("sprConstructor 2 (MAXOUTS=%d)\n",MAXOUTS);fflush(stdout);

  for(ii=0; ii<MAXOUTS; ii++)
  {
    printf("sprConstructor 21 (MAXOUTS=%d, ii=%d)\n",MAXOUTS,ii);fflush(stdout);

    BUFIN[ii] = (unsigned int *)calloc(MAXBUF,sizeof(int));
    if(BUFIN[ii]==NULL)
    {
      printf("Cannot allocate BUFIN[%d] - exit\n",ii);fflush(stdout);
      exit(0);
    }
    else
    {
      printf("sprConstructor: BUFIN[%d] allocated %d Mbytes at 0x%08x\n",ii,(MAXBUF/1024/1024)*4,BUFIN[ii]);fflush(stdout);
    }

    BUFOUT[ii] = (unsigned int *)calloc(MAXEVIOBUF,sizeof(int));
    if(BUFOUT[ii]==NULL)
    {
      printf("Cannot allocate BUFOUT[%d] - exit\n",ii);fflush(stdout);
      exit(0);
    }
    else
    {
      printf("sprConstructor: BUFOUT[%d] allocated %d Mbytes at 0x%08x\n",ii,(MAXEVIOBUF/1024/1024)*4,BUFOUT[ii]);fflush(stdout);
    }
  }

  printf("sprConstructor 3\n");fflush(stdout);

  /***************************/
  /***************************/

  tcpState = DA_BOOTED;
  if(codaUpdateStatus("booted") != ET_OK) return(ET_ERROR);

  printf("sprConstructor: calling tcpServer(%s, %s)\n",localobject->name, mysql_host);
  tcpServer(localobject->name, mysql_host); /*start server to process non-coda commands sent by tcpClient*/

  return(ET_OK);
}


int
sprDestructor()
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


int
codaDownload(char *conf)
{
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;
  PROC theProc;
  int have_all_inputs = 0;

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

  int res, ret, len;
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
  printf("coda_spr: configname = >%s<\n",configname);fflush(stdout);


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
  if(dbsock==NULL)
  {
    printf("ERROR: cannot connect to the database - exit\n");
    exit(1);
  }
  else
  {    
    /* default output to evio */
    printf("\n=== evio format will be used ===\n");
    etp->output_file = 0;

    sprintf(tmp,"SELECT value FROM %s_option WHERE name='dataFile'",configname);
    if(dbGetStr(dbsock, tmp, tmpp)!=CODA_OK)
    {
      printf("cannot get 'dataFile' from table >%s_option< - will not write output file\n",configname);
    }
    else
    {
      int  arg1c;
      char arg1v[LISTARGV1][LISTARGV2];
      char *p_sl;
      
      listSplit2(tmpp," ",&arg1c,arg1v);
      printf("\nfirst split, arg1c=%d, first piece >%s<\n",arg1c,arg1v[0]);
      if(arg1c==1)
      {
        strcpy(etp->filename,arg1v[0]);
        etp->usesubdir = 0;
        etp->output_file = 1;
        printf("Will use filename >%s<, no subdirectory\n\n",etp->filename);
      }
      else if(arg1c==2)
      {
        etp->usesubdir = 1;

	/* extract string after last slash */
        p_sl = strrchr(arg1v[0], '/');
        if (p_sl)
        {
          strcpy(etp->subfilename,p_sl+1);
        }
        else
        {
          printf("Cannot find any slashes - exit\n");
          exit(0);
        }

        strcpy(etp->subdirname,arg1v[0]);

        etp->output_file = 1;
        printf("Will use subdirectory >%s<, subfile >%s<\n\n",etp->subdirname,etp->subfilename);
      }
      else
      {
        printf("coda_erc: ERROR parsing datafile name - exit\n",listArgc);
        exit(0);
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



  printf("INFO: Downloading configuration '%s'\n", configname);

  etp->exit = 0;
  printf("etp->exit = %d\n",etp->exit);

  for(i=0; i<MAXOUTS; i++)
  {
    etp->procs_id[i] = 0;
  }


  /* we assume that database update by previous section is finished */

  /* create processing threads (one for each output thread) */
  for(ii=0; ii<MAXOUTS; ii++)
  {
    if(etp->procs_id[ii] == 0)
    {
      printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> sizeof(PROC_S)=%d (long int = %d, pthread_t = %d)\n",sizeof(PROC_S),sizeof(long int),sizeof(pthread_t));
      theProc = (PROC) calloc(sizeof(PROC_S),1);
      printf("INFO: allocate theProc = %llx\n",theProc);
      etp->procs[ii] = theProc;

      theProc->exit = 0;
      theProc->ithread = ii;
      //strcpy(theProc->host_in,theFb->host);
      //theProc->port_in = theFb->port_out[ii];
      theProc->connected = 0;
      theProc->output_file = 0;
      theProc->bufin = BUFIN[ii];
      theProc->bufout = BUFOUT[ii];

      theProc->index = 1; /*skip tot_len, points to the first word of actual data*/
      theProc->nsets = 0;

      /*builder area initialization*/
      theProc->nstreams_active = 0;
      theProc->nrocs_active = 0;
      for(i=0; i<NROCS; i++)
      {
        theProc->roc_id2rocid[i] = 0;
        theProc->rocid2roc_id[i] = 0;
      }


      
      {
        static void *stack_addr;
        size_t stack_size;

        /* initialize attr with default attributes */
        ret = pthread_attr_init(&attr);
        if (ret != 0)
        {
	  perror("pthread_attr_init");
	  exit(-1);
	}

        /* Get the default stack_addr and stack_size value */
	//ret = pthread_attr_getstack(&attr, &stack_addr, &stack_size);
	//if (ret != 0)
        //{
	//  perror("pthread_attr_getstack");
	//  exit(-1);
	//}
        //printf("stack_addr = %p, stack_size = %u\n", stack_addr, stack_size); does not return anything useful ...



#if 0
#define OFFSET 0x7
        stack_size = PTHREAD_STACK_MIN;
        if (posix_memalign(&stack_addr, sysconf(_SC_PAGE_SIZE), stack_size) != 0)
        {
	  perror("out of memory while allocating the stack memory");
	  exit(PTS_UNRESOLVED);
	}

	stack_addr = stack_addr + OFFSET;
	/* printf("stack_addr = %p, stack_size = %u\n", stack_addr, stack_size); */
	ret = pthread_attr_setstack(&attr, stack_addr, stack_size);
	if (ret != EINVAL)
        {
	  printf("The function didn't fail when stackaddr lacks proper alignment\n");
	}

	stack_addr = stack_addr + OFFSET;
	stack_size = PTHREAD_STACK_MIN + OFFSET;
	/* printf("stack_addr = %p, stack_size = %u\n", stack_addr, stack_size); */

	ret = pthread_attr_setstack(&attr, stack_addr, stack_size);
        if (ret != EINVAL)
        {
	  printf("The function didn't fail when (stackaddr + stacksize) lacks proper alignment\n");
	}
#endif



        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
        res = pthread_create(&etp->procs_id[ii], &attr, (void *(*)(void *)) spr_proc_thread, (void *) theProc);
        if(res!=0)
        {
          printf("ERROR: pthread_create(spr_proc_thread) returned %d - exit\n",res);
          exit(-1);
        }
        else
        {
          printf("INFO: Created 'spr_proc_thread', procs_id[%d] = 0x%08x\n",ii,etp->procs_id[ii]);
          theProc->thread = etp->procs_id[ii];
        }
      }

      
    }
  }



  /* wait for all PROCs to connect to OUTs */
  while(have_all_inputs == 0)
  {
    have_all_inputs = 1;
    for(i=0; i<MAXOUTS; i++)
    {
      if(etp->procs[i]->connected == 0)
      {
        printf("Waiting for PROC[%d] to connect to OUT[%d] ..\n",i,i);
        have_all_inputs = 0;
      }
    }

    sleep(1);

    //if(etp->exit == 1)
    //{
    //  printf("Did not get connection for PROC[%d], but received 'Exit' command - returning from Prestart\n",i);
    //  return(ET_ERROR);
    //}
  }
  printf("Got all PROCs connected\n");




  printf("coda_spr: downloaded !!!\n");

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

  if(etp->output_file==1)
  {
    /* form file name(s) for every PROC thread */
    for(ix=0; ix<MAXOUTS; ix++)
    {
      etp->procs[ix]->output_file = 1;
      sprintf(etp->procs[ix]->filename,"%s_%06d/%s_%06d.evio.%05d",etp->subdirname,object->runNumber,etp->subfilename,object->runNumber,ix);
      printf("PROC thread %05d will write to output file >%s<\n",ix,etp->procs[ix]->filename);
    }

    /* create subdirectory if it does not exist */
    sprintf(subdirname,"%s_%06d",etp->subdirname,object->runNumber);
    res = mkdir(subdirname, mode);
    if(res!=0)
    {
      printf("ERROR: cannot create subdirectory >%s< for data, mkdir returns %d - exit\n",subdirname,res);
      exit(0);
    }
    if(chmod(subdirname, mode) != 0)
    {
      printf("coda_spr: ERROR: cannot change mode on subdirectory >%s<\n",subdirname);
    }
    else
    {
      printf("coda_spr: INFO: changed mode on subdirectory >%s< - opened for everybody\n",subdirname);
    }

    //evio
    for(ix=0; ix<MAXOUTS; ix++)
    {
      etp->procs[ix]->output_handle = 0;
      if((res=evOpen(etp->procs[ix]->filename,"w",&(etp->procs[ix]->output_handle)))!=0)
      {
        etp->procs[ix]->output_handle = 0;
        printf("\n ?Unable to open output file %s, status=%d\n\n",etp->procs[ix]->filename,res);
        exit(0);
      }
      else
      {
        printf("coda_spr: INFO: opened output file >%s< for PROC %d, handle=%d\n",etp->procs[ix]->filename,ix,etp->procs[ix]->output_handle);
      }
    }
    //evio



  }
  else /* do not white output file */
  {
    for(ix=0; ix<MAXOUTS; ix++)
    {
      etp->procs[ix]->output_file = 0;
    }
  }


  /*sergey: dummy backend
  tcpState = DA_PAUSED;
  codaUpdateStatus("paused");
  return(ET_OK);
*/



  /*REDO for PROCs ???*/
  /* wait for all VTPs to connect to our threads 
  while(have_all_inputs == 0)
  {
    have_all_inputs = 1;
    for(i=0; i<etp->nthreads; i++)
    {
      if(etp->links[i]->fd_in == 0)
      {
        //printf("Waiting for input thread [%d] to accept connection from VTP %s ..\n",i,etp->links[i]->name);
        have_all_inputs = 0;
      }
    }

    sleep(1);

    if(etp->exit == 1)
    {
      printf("Did not get all connections from VTPs, but received 'Exit' command - returning from Prestart\n");
      return(ET_ERROR);
    }
  }
  printf("Got all VTPs connected\n");
  */


  /* gracefully stop spr loop */
  spr_loop_exit = 1;
  ii = 3;
  while(spr_loop_exit)
  {
    printf("download: wait for spr_loop to exit ..\n");
    sleep(1);
    ii --;
    if(ii<0) break;
  }

  if(ii<0)
  {
    printf("WARN: cannot exit spr_loop gracefully, will kill it\n");
    /* TODO: delete spr_loop thread */
    sleep(1);

    spr_loop_exit = 0; /* to let new spr_loop to start */
  }

  /* Spawn the spr_loop Thread */
  pthread_attr_init(&detached_attr);
  pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setscope(&detached_attr, PTHREAD_SCOPE_SYSTEM/*PTHREAD_SCOPE_PROCESS*/);
  pthread_create( /*(unsigned int *)*/ &iTaskROL, &detached_attr,
		   (void *(*)(void *)) spr_loop, (void *) NULL);

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
  return(ET_OK);
}

int
codaEnd()
{
  objClass object = localobject;
  ET_priv *etp = (ET_priv *) object->privated;
  int ii, jj;

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

  //if(logic_type == 0) /*SRO*/

  sleep(1);


  // ERROR: seems we sometimes are closing while evWrite still being called !!!!!!!!!!

  //evio
  if(etp->output_file==1)
  {
    printf("Calling evClose ...\n");
    for(ii=0; ii<MAXOUTS; ii++)
    {
      printf("Calling evClose for output thread [%2d]\n",ii);
      evClose(etp->procs[ii]->output_handle);
    }
  }
  //evio


  for(ii=0; ii<MAXOUTS; ii++)
  {
    etp->procs[ii]->exit = 1;
    printf("[%d] Sent exit command to proc thread ..\n",ii);fflush(stdout);
  }
  
  sleep(3);

  for(ii=0; ii<MAXOUTS; ii++)
  {
    printf("Checking proc thread ..\n");fflush(stdout);
    if(etp->procs[ii]->exit!=-1)
    {
      pthread_cancel(etp->procs_id[ii]);
      printf("proc thread canceled\n");
    }
    else
    {
      printf("proc thread already exited\n");
    }
  }
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
coda_spr(char *conf)
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

  sprConstructor();

  printf("333\n");fflush(stdout);

  if(standalone)
  {
    printf("444\n");fflush(stdout);
    coda_spr("FT1_TRIDAS");
    exit;
  }

  printf("\n\n\n sprConstructor done, running CODA_Execute\n\n");fflush(stdout);
  /* CODA_Service ("SPR"); */
  CODA_Execute ();

  sprDestructor(); /* never called ... */
}














#endif /*if defined(Linux_armv7l)*/
