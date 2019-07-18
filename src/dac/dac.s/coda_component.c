/*----------------------------------------------------------------------------*
 *  Copyright (c) 1991, 1992  Southeastern Universities Research Association, *
 *                            Continuous Electron Beam Accelerator Facility   *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 * CEBAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606 *
 *      heyes@cebaf.gov   Tel: (804) 249-7030    Fax: (804) 249-7363          *
 *----------------------------------------------------------------------------*
 * Description:
 *	class for general CODA component
 *
 * Author:
 *	Graham Heyes
 *	CEBAF Data Acquisition Group
 *
 *---------------------------------------------------------------------------*/
/* include files follow here */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <dlfcn.h>

#ifdef Linux_vme
#include "ipc.h"
#endif

#include "rc.h"
#include "rolInt.h"
#include "da.h"
#include "libdb.h"

#include "circbuf.h" /* to get NIMNETBUFS */
#include "bigbuf.h" /* to get NIMNETBUFS */

#ifdef Linux
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/prctl.h>
#endif

/*
#define CODA_ERROR 1
#define CODA_OK 0
defined in et_private.h !!!???*/
#include <et_private.h>

/* more globals */
static char *et_filename = NULL;  /* for command line */
char et_name[ET_FILENAME_LENGTH];

static int iTaskCODA;
static int iTaskUDP;

/* udp */
static int udp_loop_exit;
static int udp_loop_ready;
static int udpsocket;
static int udpport;
static char udphost[128];

/*static*/ objClass localobject;

/*
 * Start of coda globals etc... There should be no globals !!!!
 */

/*static*/ char configname[128]; /* Sergey: for CODA configuration name */




/*static*/ char    *expid = NULL;
/*static*/ char    *session = NULL;
static char    *objects = NULL;
/*static*/ char    *mysql_host = NULL;

static char Expid[80];
static char Session[80];
static char Objects[80];
static char Mysql_host[80];

static char ObjectsName[80];
static char ObjectsClass[80];

static char    *debugString = NULL;
FUNCPTR         process_poll_proc = NULL;
unsigned int *eventNumber;
uint64_t *dataSent;

int            rcdebug_level__;
int             global_code[32];
char           *global_routine[8][32];
jmp_buf         global_env[8][32];
int            global_env_depth[32];
int            use_recover = 0;
static int     codaDebugLevel = 0;



/*****************************************/
/*** some test functions for tcpServer ***/
void ctest0() {printf("test0\n");}
void ctest1(int a) {printf("test1: a=%d\n",a);}
void ctest2(int a, int b) {printf("test2: a=%d, b=%d\n",a,b);}
void ctest3(int a, float b, int c) {printf("test3: a=%d, b=%f, c=%d\n",a,b,c);}
void ctest33(int a, float b, int c, int d) {printf("test33: a=%d, b=%f, c=%d, d=%d\n",a,b,c,d);}
void ctest4(int a, float b, char *c, int d) {printf("test4: a=%d, b=%f, c=>%s<, d=%d\n",a,b,c,d);}
void ctest5(char *s) {printf("test5: s=>%s<\n",s);}
void ctest6(char ch) {printf("test6: ch=>%c<\n",ch);}
void ctest7(float ff) {printf("test7: ff=>%f<\n",ff);}

void tests()
{
  printf("Sizes: int=%d(%d) float=%d(%d)\n",sizeof(int),sizeof(int*),sizeof(float),sizeof(float*));
}




/*******************************************************/
/*******************************************************/
/* following function provide heartbeat information */
/* heartbeat status array and function prototypes */

static int heartbeat[HB_MAX];
static int heartbeatold[HB_MAX];
static int heartbeatcount[HB_MAX];
static int heartmask[HB_MAX];
static int heartbeaterrorflag[HB_MAX];
static int heartbeaterrormsgcount[HB_MAX];
static int heartbeaterrormsg[HB_MAX][130];


/* called by main controlling thread once */
int
resetHeartBeats()
{
  int i;
  for(i=0; i<HB_MAX; i++)
  {
    heartbeat[i] = heartbeatold[i] = time(0);
    heartbeatcount[i] = -1; /* do not check */
    heartmask[i] = 0;
    heartbeaterrorflag[i] = 0;
    heartbeaterrormsgcount[i] = 0;
  }
  return(0);
}

/* called by monitored threads;
it sets appropriate bits in 'heartmask' amd sets countdown; 'checkheartbeat'
will decrement countdown counter every second and when it reaches 0 next check
will be performed */
int
setHeartBeat(int system, int bit, int countdown)
{
  if(bit<0 || bit>31)
  {
    printf("ERROR in setHeartBeat - do nothing\n");
    return(-1);
  }
  else
  {
    if(countdown<4) countdown = 4;
    /*printf("setHeartBeat: %d %d %d\n",system,bit,countdown)*/;
  }

  heartbeat[system] = time(0);
  heartbeatcount[system] = countdown;

  /*
  if(bit<=0) heartmask[system] = 1;
  else       heartmask[system] |= (1<<bit);
  */
  if(bit<=0) heartmask[system] = 0;
  else       heartmask[system] = bit;

  return(0);
}


/* returns 0 if everything fine, 1 if there is no heartbeat */
int
getHeartBeat(int system)
{
  return(heartbeaterrorflag[system]);
}


/* called by main controlling thread in a loop */
int
checkHeartBeats()
{
  int i;
  char tmp[1000];
  /*
  for(i=0; i<HB_MAX; i++) heartbeat[i] = localpmc.heartbeat[i];
  */
  for(i=0; i<HB_MAX; i++)
  {
    if(heartbeaterrormsgcount[i]>0)
	{

      /*strcpy(tmp,"err:"); send INFO, do not scare shift takers ..*/
      strcpy(tmp,"inf:");

      strcat(tmp,localobject->name);
      strcat(tmp," ");
      strcat(tmp,(char *)&heartbeaterrormsg[i][0]);

      UDP_request(tmp);

      heartbeaterrormsgcount[i] --;
      if(heartbeaterrormsgcount[i]==0) UDP_cancel(tmp);
	}
    /*else if(heartbeatcount[i]<0)
	{
      printf("INFO: HeartBeat[%1d]: do not check (%d)\n",
        i,heartbeatcount[i]);
	}*/
    else if(heartbeatcount[i]>0)
    {
      if(heartbeatcount[i]==3)
      {
        heartbeatold[i] = heartbeat[i];
      }
	  /*
      printf("INFO: HeartBeat[%1d]: decrement heartbeatcount=%d(beat=%d,%d)\n",
        i,heartbeatcount[i],heartbeat[i],heartbeatold[i]);
	  */
      heartbeatcount[i] --;

      if(heartbeatcount[i]==0)
	  {
        if(heartbeat[i]==heartbeatold[i])
        {
          sprintf(tmp,"sys %d, mask %d",i,heartmask[i]);

          /* set error */
          printf("setHeartError: %d >%s<\n",i,tmp);
          heartbeaterrorflag[i] = 1;
          heartbeaterrormsgcount[i] = 2;
          strncpy((char *)&heartbeaterrormsg[i][0],tmp,128);

          printf("WARN: HeartBeat[%1d]: heartbeat=%d(%d) heartmask=%d\n",
            i,heartbeat[i],heartbeatold[i],heartmask[i]);
        }
        else
	    {
		  /*
          printf("INFO: HeartBeat[%1d]: heartbeat=%d(%d) heartmask=0x%08x\n",
            i,heartbeat[i],heartbeatold[i],heartmask[i]);
		  */
	    }
	  }
	}
  }
  return(0);
}

/*******************************************************/
/*******************************************************/
/*******************************************************/

/* returns free TCP port */
int
codaFindFreeTcpPort()
{
  int port = 0;
  int sock;
  struct sockaddr_in serv_addr;
  socklen_t len;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock < 0)
  {
    printf("socket error\n");
    return(0);
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = 0;
  if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
  {
    if(errno == EADDRINUSE)
    {
      printf("the port is not available. already to other process\n");
      return(0);
    }
    else
    {
      printf("could not bind to process (%d) %s\n", errno, strerror(errno));
      return(0);
    }
  }

  len = sizeof(serv_addr);
  if (getsockname(sock, (struct sockaddr *)&serv_addr, &len) == -1)
  {
    perror("getsockname");
    return(0);
  }

  port = ntohs(serv_addr.sin_port);

  if (close (sock) < 0 )
  {
    printf("did not close: %s\n", strerror(errno));
    return(0);
  }

  return(port);
}



/* read whole text file */
char *
loadwholefile(char *file, int *size, int *padding)
{
  FILE *fp;
  int res, nbytes;
  char *buffer;

  fp = fopen(file,"rb");
  if(!fp)
  {
    printf("loadwholefile: error opening file\n");
	return(NULL);
  }

  /* obtain the value of the file position indicator in the end of file */
  fseek(fp,0L,SEEK_END);
  nbytes = ftell(fp);
  rewind(fp);

  /* allocate memory for entire content */
  buffer = calloc(1,nbytes+10);
  if(!buffer)
  {
    fclose(fp);
    printf("loadwholefile: memory alloc fails\n");
    return(NULL);
  }
  else
  {
    printf("loadwholefile: allocated %d bytes output buffer\n",nbytes+5);
  }

  buffer[0] = '\n'; /*return in the beginning*/

  /* copy the file into the buffer */
  res = fread((char *)&buffer[1],nbytes,1,fp);
  if(res != 1)
  {
    fclose(fp);
    free(buffer);
    printf("loadwholefile: entire read fails, res=%d\n",res);
    return(NULL);
  }

  nbytes ++; /* count '\n' */

  /* add end-of-string in the end */
  buffer[nbytes] = '\0';

  nbytes ++; /* count '\0' */

  /*calculate padding */
  *padding = ((nbytes+3)/4)*4 - nbytes;

  /* fill end of buffer with \004 */
  buffer[nbytes+0] = '\4';
  buffer[nbytes+1] = '\4';
  buffer[nbytes+2] = '\4';
  buffer[nbytes+3] = '\4';
  buffer[nbytes+4] = '\4';

  /* buffer is a string contains the whole text */
  *size = (nbytes+5)/4; /* align to 4-byte boundary with al least one '\4' and return #words */
  fclose(fp);

  printf("loadwholefile: nbytes=%d, *size=%d words\n",nbytes,*size);

  return(buffer);
}


/*******************************************************/

/* routine to dynamically load and unload readout list */




int
codaLoadROL(ROLPARAMS *rolP, char *rolname, char *params)
{
  char ObjInitName[100];
  char *p0, *p1, *p2;
  int32_t nchar = 0;
  int64_t res;
  char *env, rolnametmp[256], rolnameful[256], tmp[128];


void *handle;
double (*cosine)(double);
char *error;




  /* if 'rolname' does not start from '/', '.' or '$', assume that it contains no path
	 and add default one */
  if(rolname[0]!='/'&&rolname[0]!='.'&&rolname[0]!='$')
  {
    strcpy(rolnametmp,"$CODA/$OSTYPE_MACHINE/rol/");
    strcat(rolnametmp,rolname);
  }
  else
  {
    strcpy(rolnametmp,rolname);
  }

  memset((char *) ObjInitName, 0, 100);
  /* 'strrchr' returns the pointer to the last occurrence of '/' */
  /* (actual readout list name starts after last '/') */
  if((p1 = strrchr (rolnametmp, '/')) == 0) p1 = rolnametmp;
  /* 'strrchr' returns the pointer to the last occurrence of '.' */
  /* (actual readout list name ends before last '.') */
  if((p2 = strrchr (rolnametmp, '.')) == 0) p2 = rolnametmp;
  nchar = (p2 - p1) - 1;
  if(nchar > 0)
  {
    ObjInitName[0] = '_';
    strncpy(&ObjInitName[1], (p1 + 1), nchar);
    strcat(ObjInitName, "__init");
    printf("ObjInitName >%s< \n",ObjInitName);
  }
  else
  {
    printf("ERROR: cannot extract ObjInitName from the rolname\n");
  }

#ifdef Linux

  /* resolve environment variables in rolname; go from '/' to '/' and replace
  env names starting from '$' by actual directories */
  rolnameful[0] = '\0';
  p0 = rolnametmp;
  while( (p1 = strchr(p0, '$')) != 0)
  {
    strncat(rolnameful,p0,(int)(p1-p0));
    p2 = strchr(p1, '/');
    if(p2==0)
	{
      printf("ERROR in ROL name: env var must start with '$' and end with '/'\n");
      return(CODA_ERROR);
	}
    strncpy(tmp,(char *)&p1[1],(int)(p2-p1-1));
    tmp[p2-p1-1] = '\0';
    env = getenv(tmp);
    if(env==NULL)
	{
      printf("ERROR in ROL name: env var >%s< does not exist\n",tmp);
      return(CODA_ERROR);
	}
    strcat(rolnameful,env);
    p0 = p2;
  }
  strcat(rolnameful,p0);
  printf("rolnameful >%s<\n",rolnameful);





  rolP->id = dlopen (rolnameful, RTLD_NOW | RTLD_GLOBAL);
  if(rolP->id == 0)
  {
	printf ("ERROR: dlopen failed on rol: dlerror returned >%s<\n",dlerror());
    exit(1);
  }


  /* RIGHT
#include <gnu/lib-names.h>
handle = dlopen(LIBM_SO, RTLD_LAZY);
  */

  /* WRONG */
handle = dlopen ("libm.so", RTLD_LAZY);


if (!handle)
{
  printf("TEST1: %s\n", dlerror());
  exit(1);
}
else
{
  printf("TEST1: handle=0x%016x\n",handle);
}




#else

  printf ("WARN: dynamic loading not supported\n");
  return(CODA_ERROR);

#endif


  if(rolP->id == NULL)
  {
    printf("ERROR: unable to load readout list >%s<\n",rolnameful);
    return(CODA_ERROR);
  }
  rolP->nounload = 0;

  res = (int64_t) dlsym (rolP->id, &ObjInitName[1]);
  rolP->rol_code = (VOIDFUNCPTR) res;
  if((res != (-1)) && (res != 0))
  {
    printf("INFO: >%s()< routine found, rolP->id=0x%016x, res=0x%016x\n",ObjInitName,rolP->id,res);
  }
  else
  {
    printf("ERROR: dlsym returned %lld\n",res);
    printf("ERROR: >%s()< routine not found\n",ObjInitName);
    printf("ERROR: <ObjName>__init() routine not found in >%s<\n",rolnameful);
    return(CODA_ERROR);
  }

  strncpy(rolP->usrString, params, 30);
  printf ("codaLoadROL: readout list >%s< is loaded at address 0x%016x 0x%016x\n",rolnameful,rolP->rol_code,*(rolP->rol_code));






dlerror();    /* Clear any existing error */

/*
cosine = dlsym(handle, "cos");
*/
res = (int64_t) dlsym(handle, "cos");

if ((error = dlerror()) != NULL)
{
  printf ("TEST2: %s\n", error);
  exit(1);
}


cosine = (VOIDFUNCPTR) res;
printf("TEST2: cosine=0x%016x *cosine=0x%016x\n",cosine,*cosine);


printf ("TEST3: %f\n", (*cosine)(2.0));
dlclose(handle);
 





  return(CODA_OK);
}


int
codaUnloadROL(ROLPARAMS *rolP)
{
#ifdef Linux
  if(dlclose ((void *) rolP->id) != 0)
  {
    printf("ERROR: failed to unload list %s\n",rolP->tclName);
    return(CODA_ERROR);
  }
  printf("Old ROL object module 0x%08x unloaded\n",rolP->id);
#else
  printf("WARN: dynamic loading not yet supported on this platform\n");
#endif
  return(CODA_OK);
}






/* Routine to check the Endianness of host machine */
int
isBigEndian(void)
{
  unsigned short num, switched_num;
  
  num = (2<<8) + 1;
  switched_num = htons(num);
  if(num == switched_num) return(1);
  else                    return(0);
}


void
debug_printf(int level, char *fmt,...)
{
  va_list ap;
  int severity;
  /*
printf("debug_printf reached\n");fflush(stdout);
printf("debug_printf: level=%d, fmt >%s<\n",level,fmt);fflush(stdout);
  */
  va_start(ap,fmt);
 
  if (level & codaDebugLevel) {
    fprintf(stderr, "[CODA debug] ");
    
    vfprintf (stderr,fmt, ap);
  }
  /*
printf("debug_printf done\n");fflush(stdout);
  */
}

int
pr_time(char *stuff) 
{
  double d1=0.,d2;
#if defined SOLARIS
  struct timespec to;
  clock_gettime(CLOCK_REALTIME,&to);
  d1 = to.tv_sec;
  d2 = to.tv_nsec;
  d1 = d1 + (d2/1000000000.0);
#elif defined LINUX
  struct timeval to;
  gettimeofday(&to,NULL);
  d1 = to.tv_sec;
  d2 = to.tv_usec;
  d1 = d1 + (d2/1000000.0);
#endif
  printf("%f %s\n",d1,stuff);
}
/*
 * C error recovery, we attempt to recover from (normally) fatal system errors.
 */

/**********************************************************
 * System error Recovery - save context to jump to in case
 * of Fatal system error in C code.
 **********************************************************/
 
/*
 * Pop a stored context off the stack (A stack of process contexts is saved, in
 * case of a system level error we can jump back to a safe state. This gives
 * simple error recovery..
 */
int
lastContext ()
{
  /*
#define pthread_self() 1
  if (global_env_depth[pthread_self() & 31] > 0)
    global_env_depth[pthread_self() & 31]--;
  */
  return(0);
}


/*
 * Error recovery Init. . .
 */

void *
signal_thread (void *arg)
{
  sigset_t   signal_set;
  int        sig_number, status;
  int        thr = (long) arg;
  char       *rtn;

  printf("signal_thread 1\n");fflush(stdout);

  if (global_env_depth[thr]>0)
  {
    global_env_depth[thr]--;
  }
  printf("signal_thread 2\n");fflush(stdout);
  rtn = global_routine[global_env_depth[thr]][thr];
  printf("signal_thread 3\n");fflush(stdout);
  
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGSEGV);
  sigaddset(&signal_set, SIGBUS);
  sigaddset(&signal_set, SIGILL);
  sigaddset(&signal_set, SIGFPE);
  sigaddset(&signal_set, SIGTRAP);
  sigaddset(&signal_set, SIGINT);
  sigaddset(&signal_set, SIGTERM);
  sigaddset(&signal_set, SIGHUP);

  printf("signal_thread 4\n");fflush(stdout);

  while (1)
  {
    sigwait(&signal_set, &sig_number);
    printf("signal_thread got signal number %d (%s)\n",sig_number,strsignal(sig_number));fflush(stdout);
    switch (sig_number)
    {
      case SIGSEGV:
        printf ("ERROR Segmentation fault - presumed fatal in %s\n", rtn);fflush(stdout);
        fprintf (stdout, "ERROR Segmentation fault - presumed fatal in %s\n", rtn);
        break;
    
		/* we are here when xterm closed, exit gracefully */
      case SIGHUP:
        printf ("SIGHUP in %s (xterm probably closed)\n", rtn);fflush(stdout);
	    exit(0);
        break;

      case SIGBUS:
        printf ("ERROR Bus error in %s - presumed fatal\n", rtn);fflush(stdout);
        break;
		/*
      case SIGILL:
        printf ("ERROR %s, in %s", sys_errlist[errno],rtn);
        perror ("error was-");
        break;
		*/
      case SIGFPE:
        printf ("ERROR Math error (probably 1/0) in %s\n", rtn);fflush(stdout);
        break;
    
      case SIGTRAP:
        printf ("ERROR Math error (probably 1/0) in %s\n", rtn);fflush(stdout);
        break;
         
      case SIGINT:
        printf ("ERROR got SIGINT\n");fflush(stdout);
	    exit(0);

      case SIGTERM:
		/*
        debug_printf (2,"killed by: %s", Tcl_SignalMsg (sig_number));
        Tk_doneFlag__ = 1;
		*/
        printf ("ERROR got SIGTERM\n");fflush(stdout);
        exit(0);/*return;*/
        break;
        
      default:
        printf ("ERROR Unknown signal %d in %s\n", sig_number, rtn);fflush(stdout);
        return NULL;
    }

    printf("signal_thread 5\n");fflush(stdout);

#ifdef Linux_vme
	bb_dma_free();
#endif

    printf("signal_thread 6\n");fflush(stdout);

    global_code[thr] = sig_number; 
    printf("signal_thread 7\n");fflush(stdout);
    siglongjmp(global_env[global_env_depth[thr]][thr],sig_number);
    printf("signal_thread 8\n");fflush(stdout);
  }
}


void
Recover_Init ()
{
  printf("\nRecover_Init reached\n");fflush(stdout);

#if !defined(Darwin)
  pthread_t  id;
  sigset_t   signal_set;
  int        status;
  /*int        thr = pthread_self() & 31; ??????*/
  
  sigfillset(&signal_set);
  status = pthread_sigmask(SIG_BLOCK, &signal_set, NULL);
  if (status != 0)
  {
    fprintf (stderr, "Recover_Init: error in setting signal mask, %d\n", status);
    exit (3);
  }
  else
  {
    printf("Recover_Init: set signals, status=%d\n\n",status);fflush(stdout);
  }

  printf("\nRecover_Init 1\n");fflush(stdout);

  status = pthread_create(&id, NULL, (void *(*)(void *)) signal_thread, /*(void *) thr*/NULL); /*sergey: last par ??? */
  /*status = pthread_create(&id, NULL, signal_thread, NULL);*/
  if(status!=0)
  {
    printf("Recover_Init: ERROR: pthread_create returned %d - exit\n",status);fflush(stdout);
    exit(0); 
  }
  else
  {
    printf("Recover_Init: pthread_create succeeded\n");fflush(stdout);
  }

  printf("\nRecover_Init 2\n");fflush(stdout);
  bzero(global_env_depth, sizeof(global_env_depth));
  printf("\nRecover_Init 3\n");fflush(stdout);
#endif
}


char *user_flag1 = NULL;
char *user_flag2 = NULL;
unsigned int user_flag3 = 0;
unsigned int user_flag4 = 0;


int
coda_constructor()
{
  MYSQL *dbsock;
  char tmpp[256];

printf("\n\ncoda_constructor reached\n");fflush(stdout);

  /* Allocate storage for local class information */
  localobject = (objClass) calloc (sizeof (objClassStore),1);
  bzero ((char *) localobject, sizeof (objClassStore));

  localobject->name = (char *) calloc(81,1);
  strcpy (localobject->name, ObjectsName);
  printf("object->name >%s<\n",localobject->name);

  localobject->state = (char *) calloc(81,1);

  localobject->className = ObjectsClass;
  printf("object->className >%s<\n",localobject->className);

  localobject->codaid = 0;

  printf("\nCODA version >%s<, Name >%s<, Type >%s<, Id %d\n\n",
	 VERSION,
	 localobject->name,
	 localobject->className,
	 localobject->codaid);

  eventNumber = (unsigned int *) &localobject->nevents;
  dataSent = (uint64_t *) &localobject->nlongs;

  /* set state to booted and update 'state' field in database*/
  if(codaUpdateStatus("booted") != CODA_OK) return(CODA_ERROR);
  printf("INFO: '%s' state now '%s'\n",localobject->name,localobject->state);

  dbsock = dbConnect(mysql_host, expid);
  sprintf(tmpp,"SELECT id FROM process WHERE name='%s'",localobject->name);
  if(dbGetInt(dbsock, tmpp, &localobject->codaid)==CODA_ERROR)
  {
    dbDisconnect(dbsock);
    return(CODA_ERROR);
  }
  dbDisconnect(dbsock);
  printf("-------> codaid=%d (localobject=0x%08x)\n",localobject->codaid,localobject);

  /* tells UDP_start() this is a first call */
  udp_loop_exit = -1;

  /*set signal handler*/
  Recover_Init();

  return CODA_OK;
}


/*never called ???*/
int
coda_destructor()
{
  
  if ((localobject != NULL) && (localobject->name != NULL))
  {
    printf("WARN: delete called in object '%s'\n",localobject->name);

	/*
	("database query \"UPDATE process SET inuse='no',state='down' WHERE name='",localobject->name)
	*/
  }

  /* disconnect from IPC server */
#ifdef Linux_vme
  epics_json_msg_close();
#endif
#ifdef Linux_armv7l
  epics_json_msg_close();
#endif

  return CODA_OK;
}





int
listSplit2(char *list, char *separator, int *argc, char argv[LISTARGV1][LISTARGV2])
{
  char *p, str[1024];
  strcpy(str,list);
  p = strtok(str,separator);
  *argc = 0;
  while(p != NULL)
  {
    /*printf("1[%d]: >%s< (%d)\n",*argc,p,strlen(p));*/
    strcpy((char *)&argv[*argc][0], (char *)p);
    /*printf("2[%d]: >%s< (%d)\n",*argc,(char *)&argv[*argc][0],strlen((char *)&argv[*argc][0]));*/
    (*argc) ++;
    if( (*argc) >= LISTARGV1)
	{
      printf("listSplit2 ERROR: too many args\n");
      return(0);
	}
    p = strtok(NULL,separator);
  }

  return(0);
}


/*******************************************************************/
/* main initialization routine, called from all coda_... processes */

void
CODA_Init(int argc, char **argv)
{
  char       *args, *obj, *p, buf[20], tmp[128];
  int        i, code;
  MYSQL *dbsock;
  int  listArgc;
  char listArgv[LISTARGV1][LISTARGV2];

  const char *help = "\nusage:\n\n coda_[roc/eb/er/etc]\n"
    "              [-expid Name of experiment, same as database name]\n"
    "              [-session Name of current Session]\n"
    "              [-objects Name and type of this object]\n"
    "              [-name Name of object]\n"
    "              [-mysql_host Name of host to connect to for mysql access]\n";


  /* parsing command line options: loop over all arguments, except the 1st (which is program name) */
  i = 1;
  while(i<argc)
  {
    if(strncasecmp(argv[i],"-h",2)==0)
    {
      printf("%s",help);
      exit(0);
    }
    else if (strncasecmp(argv[i],"-expid",2)==0)
    {
      strcpy(Expid,argv[i+1]);
      expid = Expid;
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-session",2)==0)
    {
      strcpy(Session,argv[i+1]);
      session = Session;
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-objects",2)==0)
    {
	  strcpy(Objects,argv[i+1]);


      /*************************************************************/
      /* if argument is just "ROC" or "TS", add hostname before it */

      if(!strcmp(Objects,"ROC"))
	  {
        strcpy(tmp,getenv("HOST"));
        if( (p=strchr(tmp,'.'))!=NULL )
		{
          printf("Will use everything before first '.' appearance in HOST=>%s<\n",tmp);
          *p = '\0';
		}
        sprintf(Objects,"%s ROC",tmp);
        printf("Received object 'ROC', will use '%s'\n",Objects);
	  }
      else if(!strcmp(Objects,"TS"))
	  {
        strcpy(tmp,getenv("HOST"));
        if( (p=strchr(tmp,'.'))!=NULL )
		{
          printf("Will use everything before first '.' appearance in HOST=>%s<\n",tmp);
          *p = '\0';
		}
        sprintf(Objects,"%s TS",tmp);
        printf("Received object 'TS', will use '%s'\n",Objects);
	  }

      objects = Objects;
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-mysql_host",2)==0)
    {
      strcpy(Mysql_host,argv[i+1]);
      mysql_host = Mysql_host;
      i=i+2;
    }
    else if (strncasecmp(argv[i],"-",1)==0)
    {
      printf("Unknown command line arg\n\n");fflush(stdout);
      printf("Unknown command line arg >%s< - exit\n\n",argv[i]);
	  exit(0);
      i=i+2;
    }
  }

  printf("CODA_Init reached, input params: >%s< >%s<\n",Session,Objects);

  if(mysql_host == NULL) mysql_host = getenv("MYSQL_HOST");
  if(expid == NULL) expid = getenv("EXPID");
  if(session == NULL) session = getenv("SESSION");

  if (objects == NULL || mysql_host  == NULL || expid == NULL || session == NULL)
  {
    printf("ERROR: objects, mysql_host, expid and session must be defined\n");
    exit(0);
  }
  else
  {
    printf("CODA_Init: use 'mysql_host' as >%s<, use 'expid' as >%s<, 'session' as >%s<, 'objects' as >%s<\n",
		   mysql_host,expid,session,objects);fflush(stdout);
  }

printf("1\n");fflush(stdout);

  /* ET name */
  if (et_filename == NULL)
  {
    /*sergey sprintf(et_name, "%s%s", "/tmp/et_sys_", session);*/
    sprintf(et_name, "/et/%s",session);
  }
  else
  {
    strncpy(et_name, et_filename, ET_FILENAME_LENGTH - 1);
    et_name[ET_FILENAME_LENGTH - 1] = '\0';
  }

printf("2\n");fflush(stdout);

#ifdef Linux_vme
  printf("CODA_Init: Connecting to IPC server ..\n");
  /*epics_json_msg_sender_init(getenv("EXPID"), getenv("SESSION"), "daq", "HallB_DAQ");*/
  epics_json_msg_sender_init("clasrun", "clasprod", "daq", "HallB_DAQ");
  printf(".. done connecting to IPC server.\n");
#endif
#ifdef Linux_armv7l
  printf("CODA_Init: Connecting to IPC server ..\n");
  /*epics_json_msg_sender_init(getenv("EXPID"), getenv("SESSION"), "daq", "HallB_DAQ");*/
  epics_json_msg_sender_init("clasrun", "clasprod", "daq", "HallB_DAQ");
  printf(".. done connecting to IPC server.\n");
#endif

printf("3\n");fflush(stdout);

  dbsock = dbConnect(mysql_host, expid);

printf("333333333333333\n");fflush(stdout);

  /* It would be nice here to set the uid and gid of the process to 
     match that of whoever is running RunControl */
  {
    static char tmp[256], tmpp[256], list[256];

    sprintf(tmp,"SELECT owner FROM sessions WHERE name='%s'",session);
    if(dbGetStr(dbsock, tmp, tmpp)==CODA_ERROR)
    {
      printf("cannot get 'owner' from table sessions for the name>%s<\n",session);
      dbDisconnect(dbsock);
      return;
    }
    else
    {
	  printf(">>>>>>>%d<<<<<<<<< %d\n",strlen(tmpp));fflush(stdout);
	  printf(">>>>>>>%s<<<<<<<<< %d\n",tmpp);fflush(stdout);
	  /*>>>>>>>clon00 boiarino 1538 146<<<<<<<<< 24*/

      dbDisconnect(dbsock);


      if(strlen(tmpp) > 2)
      {
        strncpy(list,tmpp,strlen(tmpp));
        list[strlen(tmpp)] = '\0';
        printf("list >%s<\n",list);
		/*list >clon00 boiarino 1538 146<*/
      }
      else
      {
        printf("ERROR: No owner information from database table sessions\n");
        printf("string length of tmpp = %d\n",strlen(tmpp));
        exit(3);
      }


	  listSplit2(list," ",&listArgc,listArgv);
      if(listArgc == 4)
      {
	    /*setuid(atoi(listArgv[2]));
	      setgid(atoi(listArgv[3]));*/
	  }
      else
      {
	    printf ("WARNING: Could not get uid and gid info from database\n");
	    printf ("         number of args in the id entry of sessions is %d\n",listArgc);
	  }

    }

  }


  
  {
    int ix;
    int  listArgc;
    char listArgv[LISTARGV1][LISTARGV2];	

	listSplit2(objects," ",&listArgc,listArgv);


    for(ix=0; ix<listArgc; ix+=2)
    {
	  
      char *class = listArgv[ix+1];
      obj = listArgv[ix];
      printf("session: >%s<\n",session);
      printf("class:   >%s<\n",class);
      printf("obj:     >%s<\n",obj);
	  

      strcpy(ObjectsName,listArgv[ix]);
      strcpy(ObjectsClass,listArgv[ix+1]);
      printf("session:      >%s<\n",session);/*clastest*/
      printf("object name:  >%s<\n",ObjectsName);/*croctest4*/
      printf("object class: >%s<\n",ObjectsClass);/*TS*/


	  /* set 'type' field in 'process' table if ROC or TS */
      if( (!strcmp(ObjectsClass,"ROC")) || (!strcmp(ObjectsClass,"TS")) )
	  {
        dbsock = dbConnect(mysql_host, expid);
        printf("44444444444444\n");fflush(stdout);
        sprintf(tmp,"UPDATE process SET type='%s' WHERE name='%s'",ObjectsClass,ObjectsName);
        printf("DB update: >%s<\n",tmp);
        if(mysql_query(dbsock, tmp) != 0)
        {
          printf("DB update ERROR - exit\n");
          exit(0);
        }
        dbDisconnect(dbsock);
	  }


printf("CODA_Init 13: coda_constructor starts\n");fflush(stdout);
      coda_constructor();
printf("CODA_Init 14: coda_constructor ends\n");fflush(stdout);
    }
  }

  return;
}




/* Sergey */
static int CODAtcpServer(void);




/*************************************************/
/* main loop: called from all coda_... processes */

void
CODA_Execute ()
{
  pthread_t id;
  int fd;
  int status;

  /* Sergey: start CODAtcpServer as thread */
  {
    pthread_attr_t attr;

    pthread_attr_init(&attr); /* initialize attr with default attributes */
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    status = pthread_create(&id, &attr, (void *(*)(void *)) CODAtcpServer, NULL);
    if(status!=0)
    {
      printf("CODA_Execute: ERROR: pthread_create returned %d - exit\n",status);fflush(stdout);
      exit(0); 
    }
    else
    {
      printf("CODA_Execute: pthread_create succeeded\n");fflush(stdout);
    }
  }

  /*
   * TODO: die gracefully.  This function never returns.
   */

  while(1) sleep(1);

  exit(0);
}
    










  /*************************************************/
  /*************************************************/
  /*************************************************/
  /*************************************************/
  /*************************************************/
  /* Sergey: CLAS stuff  */




  
/*
 *-------------------------------------------------------------------------*
 *   Routine to take a Data buffer in CODA format and perform a
 *   a byte swap based on the data type in the Bank structure headers.
 *
 *   Data is maniputlated in the existing buffer so that function
 *   irreversably mangles the data. If there is an error in the Bank
 *   structure the function returns -1, otherwize it returns 0;
 *   
 *-------------------------------------------------------------------------*
 */
int
CODA_bswap(int32_t *cbuf, int32_t ndata)
{
    int ii, jj, ix;
    int tlen, blen, dtype;
    int lwd;
    short shd;
    char cd;
    char *cp;
    short *sp;
    int *lp;

    ii = 0;
    while (ii<ndata) {
      lp = (int *)&cbuf[ii];
      blen = cbuf[ii] - 1;
      dtype = ((cbuf[ii+1])&0xff00)>>8;
      lwd = LSWAP(*lp);    /* Swap the length      */
      *lp++ = lwd;         
      lwd = LSWAP(*lp);    /* Swap the bank header */
      *lp++ = lwd;       
      ii += 2;
      if(dtype != DT_BANK) {
	switch(dtswap[dtype]) {
	case 0:
	  /* No swap */
	  ii += blen;
	  break;
	case 1:
	  /* short swap */
	  sp = (short *)&cbuf[ii];
	  for(jj=0; jj<(blen<<1); jj++) {
	    shd = SSWAP(*sp);
	    *sp++ = shd;
	  }
	  ii += blen;
	  break;
	case 2:
          /* int swap */
	  lp = (int *)&cbuf[ii];
	  for(jj=0; jj<blen; jj++) {
	    lwd = LSWAP(*lp);
	    *lp++ = lwd;
	  }
	  ii += blen;
	  break;
	case 3:
	  /* double swap - Sergey: WRONG */
	  lp = (long *)&cbuf[ii];
	  for(jj=0; jj<blen; jj++) {
	    lwd = LSWAP(*lp);
	    *lp++ = lwd;
	  }
	  ii += blen;
	  break;
	default:
	  /* No swap */
	  ii += blen;
	}
      }
    }
 return(0);
}




/* returns session name */
void
getSessionName(char *sessionName, int lname)
{
  int len;

  len = strlen(session);
  /*printf("getSessionName: len %d, available %d\n",len,lname-1);*/
  len = len < (lname-1) ? len : (lname-1);
  /*printf("getSessionName: returns %d symbols\n",len);*/

  strncpy(sessionName,session,len);
  sessionName[len] = '\0';
  /*printf("getSessionName: return session name >%s<\n",sessionName);*/

  return;
}



/* returns confFile from database */
void
getConfFile(char *configname, char *conffile, int lname)
{
  MYSQL *dbsock;
  char tmp[1000];
  char tmpp[1000];

  /* connect to database */
  dbsock = dbConnect(mysql_host, expid);

  sprintf(tmp,"SELECT value FROM %s_option WHERE name='confFile'",configname);
  if(dbGetStr(dbsock, tmp, tmpp)==CODA_ERROR)
  {
    printf("cannot get 'confFile' from table >%s_option<\n",configname);
    return;
  }
  else
  {
    strncpy(conffile,tmpp,lname);
    printf("got conffile >%s<\n",conffile);
  }

  /* disconnect from database */
  dbDisconnect(dbsock);
}



/*****************/
/* UDP functions */


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static pthread_mutex_t udp_lock;

typedef struct udpstruct
{
  int  active;          /* if 1 - send message; if 0 - do not send */
  int  timeinterval;    /* how often to send (sec) (not in use yet) */
  char message[1024];   /* message to be sent */

} UDPSTRUCT;

#define MAXUDPS 2
static UDPSTRUCT udpstr[MAXUDPS];



/* create a connection to given host and port; returns socket number
or 0 if connection was not established; host and port usually obtained
from 'process' table of database where name is session name (clasprod etc) */
int
UDP_establish(char *host, int port)
{
  struct hostent *hp, *gethostbyname();
  struct sockaddr_in sin;
  int s, slen;
  int socketnum;

  /* */
  bzero((char *)&sin, sizeof(sin));

  hp = gethostbyname(host);
  if(hp == 0 && (sin.sin_addr.s_addr = inet_addr(host)) == -1)
  {
	printf("UDP_establish: unknown host >%s<\n",host);
	return(0);
  }
  if(hp != 0) bcopy(hp->h_addr, &sin.sin_addr, hp->h_length);
  sin.sin_port = htons(port);
  sin.sin_family = AF_INET;

  /* create a socket (use 'SOCK_STREAM' for TCP, 'SOCK_DGRAM' for UDP */
  s = socket(AF_INET, SOCK_DGRAM, 0);
  if(s < 0)
  {
    printf("UDP_establish: cannot open socket\n");
    return(0);
  }
  else
  {
    socketnum = s;
    printf("UDP_establish: socket # %d\n",socketnum);
  }


  /**********************/
  /* set socket options */
  /**********************/

  /* socket buffer size */

  slen = 48000/*29200*/;
  if(setsockopt (s, SOL_SOCKET, SO_SNDBUF, &slen, sizeof(slen)) < 0)
  {
	printf("UDP_establish: setsockopt SO_SNDBUF failed\n");
	return(0);
  }

  {
    int nbytes, lbytes;

    nbytes = 0;
    lbytes = 4;

    getsockopt(s, SOL_SOCKET, SO_SNDBUF, (int *) &nbytes, &lbytes); 
    printf("UDP_establish: socket buffer size is %d(0x%08x) bytes\n",
      nbytes,nbytes);
  }

  {
    int optval, lbytes;

	optval = 1; /* 1-yes, 0-no */
	/*make sence for TCP only, not for UDP
    if(setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0)
	{
	  printf("UDP_establish: setsockopt SO_KEEPALIVE failed\n");
	  return(0);
	}

    optval = 0;
    lbytes = 4;
    getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (int *) &optval, &lbytes);
    printf("UDP_establish: keepAlive is %d\n",optval);
	*/
  }

#ifdef VXWORKS_needheaderfiles
  {
    int optval = 0;
    if(setsockopt (s, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof (optval)) < 0)
    {
	  printf("UDP_establish: setsockopt TCP_NODELAY failed\n");
      return(0);
    }
  }
#endif

  /* connect */
  if(connect(s, (const struct sockaddr *)&sin, sizeof (sin)) < 0)
  {
    printf("UDP_establish: connect failed: host %s port %d\n",
      inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
fflush(stdout);
exit(0);
	return(0);
  }

  printf("UDP_establish: socket %d is ready: host %s port %d\n",
      socketnum, inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

  pthread_mutex_init(&udp_lock, NULL);

  return(socketnum);
}


int
UDP_close(int socket)
{
  int retsocket;

  /* wait a while to let a few messages to be send */
    sleep(3);

  /* reset all messages first to cleanup static structure */
  UDP_reset();

  pthread_mutex_unlock(&udp_lock);
  pthread_mutex_destroy(&udp_lock);

  if(socket != 0)
  {
    if(shutdown(socket, 2)==0)
    {
      printf("UDP_close: socket #%d connection closed\n",socket);
      close(socket);
      retsocket = 0;
    }
    else
    {
      printf("UDP_close: ERROR in socket #%d connection closing\n",
        socket);
      retsocket = socket;
    }
  }

  return(retsocket);
}

/* request 'standard' UDP message to be send */
int
UDP_standard_request(char *name, char *state)
{
  char tmp[1000];

  strcpy(tmp,"sta:");
  strcat(tmp,name);
  strcat(tmp," ");
  strcat(tmp,state);

  printf("UDP_standard_request >%s<",tmp);
  printf("\n");
  printf("UDP_standard_request >%s<",tmp);
  printf("\n");
  printf("UDP_standard_request >%s<",tmp);
  printf("\n");
  printf("UDP_standard_request >%s<",tmp);
  printf("\n");
  printf("UDP_standard_request >%s<",tmp);
  printf("\n");
  printf("UDP_standard_request >%s<",tmp);
  printf("\n");

  UDP_request(tmp);

  return(0);
}



/* UDP_user_request */
int
UDP_user_request(int msgclass, char *name, char *message)
{
  char tmp[1000];

  if(msgclass==MSGERR)      strcpy(tmp,"err:");
  else if(msgclass==MSGWRN) strcpy(tmp,"wrn:");
  else                      strcpy(tmp,"inf:");

  strcat(tmp,name); /*object->name*/
  strcat(tmp," ");
  strcat(tmp,message);

  printf("UDP_user_request >%s<",tmp);
  printf("\n");

  UDP_request(tmp);

  sleep(2);

  UDP_cancel(tmp);

  return(0);
}












/* remove all UDP messages */
int
UDP_reset()
{
  int i;

  pthread_mutex_lock(&udp_lock);

  printf("UDP_reset reached\n");
  /* cancel all messages */
  for(i=0; i<MAXUDPS; i++)
  {
    printf("UDP_cancel: cancel >%s<\n",udpstr[i].message);
    udpstr[i].active = 0;
    udpstr[i].message[0] = '\0'; /* just in case */
  }

  pthread_mutex_unlock(&udp_lock);

  return(0);
}

/* cancel UDP message */
int
UDP_cancel(char *str)
{
  int i;

  pthread_mutex_lock(&udp_lock);

  /* cancel all messages with identical key (first 4 characters) */
  for(i=0; i<MAXUDPS; i++)
  {
    if(!strncmp(udpstr[i].message, str, 4))
    {
      printf("UDP_cancel: cancel >%s<\n",udpstr[i].message);
      udpstr[i].active = 0;
      udpstr[i].message[0] = '\0'; /* just in case */
      /*break;*/ /*scan whole structure in case if more then one message*/
    }
  }

  pthread_mutex_unlock(&udp_lock);

  return(0);
}

/* request UDP message to be send */
int
UDP_request(char *str)
{
  int i, found;

  /* cancel all messages with the same key (first 4 characters) */
  UDP_cancel(str);

  pthread_mutex_lock(&udp_lock);

  found = 0;
  for(i=0; i<MAXUDPS; i++)
  {
    if(udpstr[i].active == 0)
    {
      found = 1;
      udpstr[i].active = 1;
      strcpy(udpstr[i].message, str);
      break;
    }
  }

  pthread_mutex_unlock(&udp_lock);

  if(found) return(0);

  printf("UDP_request: no empty slots, message was not accepted\n");
  return(1);
}


/* returns number of data bytes written or -1 if error */
/* 'str' must be string */
int
UDP_send(int socket)
{
  char tmp[1000], tmpp[1000];
  char name[100];
  int i, nevents, eventdiff;
  int64_t nlongs;
  time_t newtime, timediff;
  float event_rate, data_rate;
  float data[4];
  static int oldevents;
  static int64_t oldlongs;
  static time_t oldtime;
  int nbytes, cc, rembytes;
  char *buffer2;

  if(oldtime==0) oldtime=time(0);

  pthread_mutex_lock(&udp_lock);

  for(i=0; i<MAXUDPS; i++)
  {
	/*printf("UDP_send[%d]: active=%d\n",i,udpstr[i].active);*/
    if(udpstr[i].active==0) continue;

    /* for the message started from 'sta:' update statistic info */
    if( !strncmp(udpstr[i].message,"sta:",4) )
    {
      nevents = *eventNumber;
      nlongs = *dataSent;

      newtime = time(0); /* time in seconds */

      event_rate = data_rate = 0.0;

      timediff = newtime - oldtime;
      eventdiff = nevents - oldevents;
	  /*printf("timediff: %u (%u - %u)\n",timediff,newtime,oldtime);*/
      if(timediff != 0)
      {
		if(eventdiff != 0)
	    {
          event_rate = eventdiff/timediff;
  	      /*printf("event_rate: %f (%u - %u)\n",event_rate,nevents,oldevents);*/
          data_rate = 4*(nlongs - oldlongs)/timediff;

          oldlongs = nlongs;
          oldevents = nevents;
          oldtime = newtime;

	      strcpy(tmp,udpstr[i].message);
          sprintf(tmpp," %d %9.3f %lld %12.3f",nevents,event_rate,nlongs,data_rate);
	      /*printf("UDP_send[%d]: %d %9.3f %d %12.3f\n",i,nevents,event_rate,nlongs,data_rate);*/
          strcat(tmp,tmpp);
		}
        else if(timediff>=3) /* if 3 seconds without rate, send message with zero rates: */
		{                    /* have to send something to make runcontrol happy          */
	      strcpy(tmp,udpstr[i].message);
          sprintf(tmpp," %d %9.3f %lld %12.3f",nevents,event_rate,nlongs,data_rate);
	      /*printf("UDP_send[%d]: %d %9.3f %d %12.3f\n",i,nevents,event_rate,nlongs,data_rate);*/
          strcat(tmp,tmpp);
		}
        else
		{
          tmp[0] = '\0'; /* do not send anything */
		}
      }
      else
	  {
        tmp[0] = '\0'; /* do not send anything */
	  }

#ifdef Linux_vme
      /* send AMQ message */
      sprintf(name,"STA:%s",localobject->name);
      data[0] = (float)nevents;
      data[1] = event_rate;
      data[2] = (float)((nlongs*4)/1048576);
      data[3] = data_rate/1048576;
      epics_json_msg_send(name, "float", 4, data);
#endif

#ifdef Linux_armv7l
      sprintf(name,"STA:%s",localobject->name);
      data[0] = (float)nevents;
      data[1] = event_rate;
      data[2] = (float)((nlongs*4)/1048576);
      data[3] = data_rate/1048576;
      epics_json_msg_send(name, "float", 4, data);
#endif

    }
    else
    {
      strcpy(tmp,udpstr[i].message); /* not 'sta:' messages */
    }

	/*
    printf("UDP_send[%d] >%s<\n",i,tmp);
    printf(" udpport=%d\n",udpport);
	*/

    nbytes = strlen(tmp);
    if(nbytes == 0)
    {
      /*printf("UDP_send: no data - return\n");*/
      goto exit3;
    }

    nbytes++; /* add one char for the end of string sign */
    rembytes = nbytes;
    buffer2 = tmp;
    while(rembytes)
    {
retry3:

      cc = write(socket,buffer2,rembytes);
	  /*cc = send(socket,buffer2,rembytes,NULL);*/
/*printf("2: %d %d\n",rembytes,cc);*/

      if(cc == -1)
      {
        if(errno == EWOULDBLOCK)
        {
          printf("UDP_send: Operation would block 2: retry ...\n");
          goto retry3;
        }

        if(errno==ENOBUFS) printf("UDP_send: No buffer space available (errno=%d)\n",errno);
        else if(errno==EPIPE) printf("UDP_send: Broken pipe (errno=%d)\n",errno);
        else if(errno==EHOSTDOWN) printf("UDP_send: Host is down (errno=%d)\n",errno);
        else if(errno==ECONNREFUSED)
        {
          /*if rcServer dies, mv2400/mv5500/mv6100 reports following message,
          while mv5100 does not !!!; I think mv5100 is right and others are
          wrong, but must figure it out ... */

          /* following message will be printed if rcServer killed, vety annoying - comment it out */
		  /*
          printf("UDP_send: Connection to host %s port %d refused (udpsocket=%d, errno=%d)\n",udphost,udpport,udpsocket,errno);
		  */
          ;
        }
        else printf("UDP_send: Unknown error errno=%d\n",errno);




/* !!! if errno==61(ECONNREFUSED), must try to reestablish connection (rcServer died) */



/*
netStackDataPoolShow - not enough buffers !!!
netStackSysPoolShow
*/
        nbytes = -1;
        goto exit3;
      }

      buffer2 += cc;
      rembytes -= cc;
      if(rembytes!=0) printf("WARN: UDP_send: rembytes=%d\n",rembytes);
    }

exit3:
    ;

  } /* for() */

  pthread_mutex_unlock(&udp_lock);

  return(nbytes);
}

void
UDP_show()
{
  int i;

  for(i=0; i<MAXUDPS; i++)
  {
    if(udpstr[i].active)
      printf("udpstr[%1d].message >%s<\n",i,udpstr[i].message);
  }
}


int
codaUpdateStatus(char *status)
{
  objClass object = localobject;
  MYSQL *dbsock;
  char tmp[1000];

  /* update database */
  printf("codaUpdateStatus: dbConnecting ..\n");fflush(stdout);
  dbsock = dbConnect(mysql_host, expid);
  if(dbsock==NULL)
  {
    printf("cannot connect to the database 7 - exit\n");
    exit(0);
  }
printf("codaUpdateStatus: dbConnect done\n");fflush(stdout);

/* update state in 'process' table */
  sprintf(tmp,"UPDATE process SET state='%s' WHERE name='%s'",
    status,localobject->name);
  printf("codaUpdateStatus(table 'process'): >%s<\n",tmp);
  if(mysql_query(dbsock, tmp) != 0)
  {
    printf("codaUpdateStatus(table 'process'): ERROR\n");
    return(CODA_ERROR);
  }

  /* if we are Event Builder, update 'log_name' in 'sessions' table,
    use it to keep 'state' for external user's information */
  if(!strcmp(localobject->className,"EB"))
  {
    sprintf(tmp,"UPDATE sessions SET log_name='%s' WHERE name='%s'",
      status,session);
    printf("codaUpdateStatus(table 'sessions'): >%s<\n",tmp);
    if(mysql_query(dbsock, tmp) != 0)
    {
      printf("codaUpdateStatus(table 'sessions'): ERROR\n");
      /*return(CODA_ERROR);*/
    }
  }

printf("codaUpdateStatus: dbDisconnecting ..\n");fflush(stdout);
  dbDisconnect(dbsock);
printf("codaUpdateStatus: dbDisconnect done\n");fflush(stdout);
  
  /* update request record */
printf("codaUpdateStatus: updating request ..\n");fflush(stdout);
  UDP_standard_request(localobject->name,status);
printf("codaUpdateStatus: updating request done\n");fflush(stdout);


  /* TODO: need also to keep status in some local variable .. */
  /*tcpState ? rocp->state ? */
  strcpy(localobject->state,status);


  return(CODA_OK);
}


/* that function sends our status to rcServer every second */
void
UDP_loop()
{
#ifdef Linux
  prctl(PR_SET_NAME,"coda_udp");
#endif

  udpsocket = UDP_establish(udphost, udpport);
  printf("udpsocket=%d\n",udpsocket);

  printf("UDP_loop started\n");fflush(stdout);
  printf("UDP_loop started\n");fflush(stdout);
  printf("UDP_loop started\n");fflush(stdout);
  printf("UDP_loop started\n");fflush(stdout);
  printf("UDP_loop started\n");fflush(stdout);

  udp_loop_ready = 1;

  /* initialize heartbeats */
  resetHeartBeats();

  while(1)
  {
    if(udp_loop_exit)
    {
      break;
    }

    /* this is appropriate place to check if all others are running fine */
    checkHeartBeats();

    UDP_send(udpsocket);

    /* wait 1 sec */
    sleep(1);
  }

  UDP_close(udpsocket);
  udp_loop_exit = 0;

  printf("UDP_loop: calls pthread_exit\n");
  pthread_exit(NULL);

  return;
}

/* start thread to report our status to rcServer's UDP port */
int
UDP_start()
{
  MYSQL *dbsock;
  char tmpp[512];
  int ii, iii;

  printf("UDP_start 111\n");fflush(stdout);

  dbsock = dbConnect(mysql_host, expid);
  if(dbsock==NULL)
  {
    printf("UDP_start: cannot connect to the database - exit\n");
    exit(0);
  }

  printf("112\n");fflush(stdout);
  sprintf(tmpp,"SELECT host FROM process WHERE name='%s'",session);
  printf("112-1\n");fflush(stdout);
  printf("112-2 >%s<\n",tmpp);fflush(stdout);
  if(dbGetStr(dbsock, tmpp, udphost)==CODA_ERROR) return(CODA_ERROR);
  printf("113\n");fflush(stdout);

  sprintf(tmpp,"SELECT port FROM process WHERE name='%s'",session);
  if(dbGetInt(dbsock, tmpp, &udpport)==CODA_ERROR) return(CODA_ERROR);

  printf("114\n");fflush(stdout);
  printf("download: UDP host is >%s< port id %d\n",udphost,udpport);fflush(stdout);

  dbDisconnect(dbsock);



  /* if it is NOT first call, exit UDP loop and start new one */
  if(udp_loop_exit != -1)
  {
    udp_loop_exit = 1;
    printf("UDP_start: exiting udp_loop\n");

    ii = 6;
    while(udp_loop_exit)
    {
      printf("UDP_start: wait for udp_loop to exit ..\n");
      sleep(1);
      ii --;
      if(ii<0) break;
    }

    if(ii<0)
	{
      printf("UDP_start: WARN: cannot exit udp_loop gracefully, will kill it\n");
      /* TODO: delete udp_loop thread */
      sleep(1);
      udp_loop_exit = 0; /* to let new UDP_LOOP to start */
	}

  }

  /* just in case */
  udp_loop_exit = 0;


  {
    int iii;
    int res;
    pthread_t thread1;
    pthread_attr_t detached_attr;

    pthread_attr_init(&detached_attr);
    pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(&detached_attr, PTHREAD_SCOPE_SYSTEM);


    udp_loop_ready = 0;
    iii = 10;

    res = pthread_create(&thread1, &detached_attr, (void *(*)(void *)) UDP_loop, (void *) NULL);
    if(res!=0)
    {
      printf("UDP_start: ERROR: pthread_create returned %d - exit\n",res);fflush(stdout);
      perror("UDP_start: pthread_create");
      exit(0); 
    }
    else
    {
      printf("UDP_start: pthread_create succeeded\n");fflush(stdout);
    }


    while((udp_loop_ready==0) && (iii>0))
	{
      printf("UDP_start: waiting for udp_loop to start %d sec ...\n",iii);
      sleep(1);
      iii --;
	}

    if(udp_loop_ready) printf("UDP_start: udp_loop started\n");
    else
	{
      printf("FATAL ERROR: UDP_start: udp_loop could not start !!!!!!!!!!!!\n");
      printf("FATAL ERROR: UDP_start: udp_loop could not start !!!!!!!!!!!!\n");
      printf("FATAL ERROR: UDP_start: udp_loop could not start !!!!!!!!!!!!\n");
      printf("FATAL ERROR: UDP_start: udp_loop could not start !!!!!!!!!!!!\n");
      printf("FATAL ERROR: UDP_start: udp_loop could not start !!!!!!!!!!!!\n");
      return(-1);
	}

  }

  return(0);
}





/***************************************************/
/***************************************************/
/***************************************************/
/* CODAtcpServer functions - to receive commands from runcontrol */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define ERROR (-1)


#include "libtcp.h" 

/* readiness flag */
static int coda_request_in_progress;
/* currently processed message */
static char coda_current_message[REQUEST_MSG_SIZE];
/* function declarations */ 
static void CODAtcpServerWorkTask(TWORK *targ); 

/**************************************************************************** 
* * CODAtcpServer - accept and process requests over a TCP socket 
* * This routine creates a TCP socket, and accepts connections over the socket 
* from clients. Each client connection is handled by spawning a separate 
* task to handle client requests. 
* * This routine may be invoked as follows: 
* -> sp CODAtcpServer
* task spawned: id = 0x3a6f1c, name = t1 
* value = 3829532 = 0x3a6f1c 
* -> MESSAGE FROM CLIENT (Internet Address 150.12.0.10, port 1027): 
* Hello out there 
* * RETURNS: Never, or ERROR if a resources could not be allocated. */ 

int
CODAtcpServer(void)
{
  struct sockaddr_in serverAddr; /* in (it's me) */
  struct sockaddr_in clientAddr; /* from */
  int sockAddrSize;              /* size of socket address structure */ 
  int sFd;                       /* socket file descriptor */ 
  int ix = 0;                    /* counter for work task names */
  int portnum = SERVER_PORT_NUM; /* desired port number; can be changed if that number in use enc */
  char workName[16];             /* name of work task */ 
  static TWORK targ;
  MYSQL *dbsock;
  char tmp[1000], temp[100];

#ifdef Linux
  prctl(PR_SET_NAME,"coda_tcp");
#endif

  /* some cleanup */
  sockAddrSize = sizeof(struct sockaddr_in);
  bzero((char *)&serverAddr, sockAddrSize); 
  bzero((char *)&clientAddr, sockAddrSize); 

  /* creates an endpoint for communication and returns a socket file descriptor */
  if((sFd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
  {
    perror("CODAtcpServer::socket"); 
    return(ERROR); 
  } 

  /* set up the local address */ 
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(portnum); /* set desired port number */
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* create a TCP-based socket (???) */ 


  /* bind socket to local address */
  while(bind(sFd, (struct sockaddr *)&serverAddr, sockAddrSize) == ERROR)
  {
    perror("CODAtcpServer::bind");

    /* try another port (just increment on one) */
    portnum ++;
    printf(" ... trying port %d\n",portnum);
    if((portnum-SERVER_PORT_NUM) > 200)
    {
      close(sFd); 
      return(ERROR);
    }

    serverAddr.sin_port = htons(portnum);
  }
  printf("CODAtcpServer: bind on port %d\n",portnum);

  /* create queue for client connection requests */ 
  if(listen(sFd, SERVER_MAX_CONNECTIONS) == ERROR)
  {
    perror("listen"); 
    close(sFd); 
    return(ERROR); 
  }

  /* update database with port number */
  dbsock = dbConnect(mysql_host, expid);
  sprintf(temp,"%d",portnum);

  /* use 'inuse' field; replace 'inuse' by 'port' when DP_ask not in use !!! */
  sprintf(tmp,"UPDATE process SET inuse='%s' WHERE name='%s'",
    temp,localobject->name);

  printf("CODAtcpServer: DB update: >%s<\n",tmp);
  if(mysql_query(dbsock, tmp) != 0)
  {
    printf("CODAtcpServer: DB update: ERROR\n");
    return(ERROR);
  }
  dbDisconnect(dbsock);

  coda_request_in_progress = 0;
  /* accept new connect requests and spawn tasks to process them */ 
  while(1)
  {


    /* do not accept new request if current one is not finished yet; too
    many requests may create network buffer shortage */
    if(coda_request_in_progress)
    {
      printf("CODAtcpServer: wait: coda request >%s< in progress\n",coda_current_message);
      sleep(1);

      continue;
    }

    /*printf("Waiting at 'accept' ..\n"); fflush(stdout);*/
    if((targ.newFd = accept(sFd, (struct sockaddr *)&clientAddr, &sockAddrSize))
          == ERROR)
    {
      perror("CODAtcpServer::accept"); 
      close(sFd); 
      return(ERROR); 
    }
    /*printf(".. 'accept' !!\n"); fflush(stdout);*/

    targ.address = inet_ntoa(clientAddr.sin_addr);
	/*
printf("WorkTask: alloc 0x%08x\n",targ.address);fflush(stdout);
printf("WorkTask: alloc 0x%08x\n",targ.address);fflush(stdout);
printf("WorkTask: alloc 0x%08x\n",targ.address);fflush(stdout);
printf("WorkTask: alloc 0x%08x\n",targ.address);fflush(stdout);
printf("WorkTask: alloc 0x%08x\n",targ.address);fflush(stdout);
	*/
    targ.port = ntohs (clientAddr.sin_port);

    coda_request_in_progress = 1;
    printf("CODAtcpServer: start work thread\n");
	{
      int ret;
	  pthread_t id;
      pthread_attr_t detached_attr;

      pthread_attr_init(&detached_attr);
      pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
      pthread_attr_setscope(&detached_attr, PTHREAD_SCOPE_SYSTEM);

      printf("CODAtcpServer: befor: socket=%d address>%s< port=%d\n",
        targ.newFd,targ.address,targ.port); fflush(stdout);

      /* block annoying IP address(es) */
	  /* is it better ???
      if( strncmp(address,"129.57.167.",11) &&
          strncmp(address,"129.57.160.",11) &&
          strncmp(address,"129.57.68.",10)  &&
          strncmp(address,"129.57.69.",10)  &&
          strncmp(address,"129.57.86.",10) &&
          strncmp(address,"129.57.29.",10) )
	  */
      if(!strncmp(targ.address,"129.57.71.",10))
	  {
        printf("CODAtcpServer: WARN: ignore request from %s\n",targ.address);
        close(targ.newFd);
        coda_request_in_progress = 0;
	  }
      else
	  {
        ret = pthread_create(&id, &detached_attr, (void *(*)(void *)) CODAtcpServerWorkTask, &targ);
        if(ret!=0)
        {
          printf("CODAtcpServer: ERROR: pthread_create(CODAtcpServerWorkTask) returned %d\n",
            ret);
          close(targ.newFd);
          coda_request_in_progress = 0;
        }
		/*
		else
		{
#ifdef Linux
      pthread_setname_np(&id, "coda_er");
#endif
	      ;
		}
		*/
	  }
	}
 
  }

  /* here we will remove port number from DB ??? */ 
  /* or do it in destructor ?? */

  return(0);
}

/**************************************************************************** 
* * CODAtcpServerWorkTask - process client requests 
* * This routine reads from the server's socket, and processes client 
* requests.
* * RETURNS: N/A. */ 


static int codaExecute(char *message);

static void
CODAtcpServerWorkTask(TWORK *targ) 
{
  TREQUEST clientRequest;            /* request/message from client */ 
  int nRead;                               /* number of bytes read */
  char message[REQUEST_MSG_SIZE];
  int itmp;
#ifdef Linux
  char thread_name[1024];
  sprintf(thread_name,"w%s:%d\0",(char *)&targ->address[6], targ->port);
  prctl(PR_SET_NAME,thread_name);
  /*prctl(PR_SET_NAME,"coda_er1");*/
#endif
  /*setHeartBeat(HB_TCP,0,2);*/

  printf("CODAtcpServerWorkTask entry\n");fflush(stdout);

  /*
  printf("CODAtcpServerWorkTask: socket=%d address>%s< port=%d\n",
    targ->newFd,targ->address,targ->port); fflush(stdout);
  */

  if( (nRead = recv(targ->newFd, (char *) &clientRequest, sizeof (TREQUEST), 0)) > 0 )
  {
    /* convert integers from network byte order */
    clientRequest.msgLen = ntohl(clientRequest.msgLen);
    clientRequest.reply = ntohl(clientRequest.reply);

	/*
    printf ("MESSAGE (nRead=%d, Address>%s<, port=%d): Executing >%s<\n", 
	    nRead, targ->address, targ->port, clientRequest.message);
	*/
    strcpy(message, clientRequest.message);

    /* store it to be used later for debugging */
    strcpy(coda_current_message, message);

    /* try Executing the message (each component must provide codaExecute() function */
    /*do not print: message may contains bad characters, it will be checked inside codaExecute
           printf("Executing >%s< (len=%d)\n",message,strlen(message));*/

	/* check if message makes sence */
    codaExecute(message);
  }
  else if(nRead == 0)
  {
    printf("connection closed, exit thread\n");
  }
  else
  {
    perror("ERROR (recv)"); 
  }

  /*free(targ->address);-stuck here !!!*/ /* free malloc from inet_ntoa() */ 
  close(targ->newFd);  /* close server socket connection */ 

  coda_request_in_progress = 0;

  printf("CODAtcpServerWorkTask exit ?\n");

  /* terminate calling thread */
  /*pthread_exit(NULL); need it ? */

  /*setHeartBeat(HB_TCP,0,-1);*/
  printf("CODAtcpServerWorkTask exit !\n");

  return;
}







/**************************/
/* some service functions */

/* flag=0: use ' ' as dividers */
/* flag=1: use {} as dividers */
int
listSplit1(char *list, int flag,
           int *argc, char argv[LISTARGV1][LISTARGV2])
{
  int i, i1, i2, len, k, ll;

  len = strlen(list);

  k = 0;
  *argc = 0;
  while(k<len)
  {
    i1 = i2 = -1;
    for(i=k; i<len; i++)
    {
      if(flag)
	  {
        if(list[i]=='{')
        {
          i1 = i;
          break;
        }
	  }
      else
      {
        i1 = i;
        while(list[i1]==' ') i1 ++; /* remove leading spaces if any */
        break;
      }
    }
    if(i1 == -1)
    {
      return(0);
    }
    if(flag) i1 ++; /* next char after { */

    for(i=i1; i<len; i++)
    {
      if(flag)
	  {
        if(list[i]=='}')
        {
          i2 = i;
          break;
        }
	  }
      else
      {
        if(i>=(len-1))
        {
          i2 = i+1;
          break;
        }
        else if(list[i]==' ')
        {
          i2 = i;
          break;
        }
      }
    }
    if(i2 == -1)
    {
      return(0);
    }
    i2 --; /* previous char before '}'  or ' ' */

    /* check for argv overflow */
    if( (*argc) >= LISTARGV1)
    {
      /*logMsg("listSplit1: too many strings in the list - earlear return\n",
        1,2,3,4,5,6);*/
      return(1);
    }
    if( (i2-i1+2) > LISTARGV2)
    {
      /*logMsg("listSplit1: too long strings in the list - earlear return\n",
        1,2,3,4,5,6);*/
      return(2);
    }

    ll = i2-i1+1;
    memcpy((char *)&argv[*argc][0], (char *)&list[i1], ll);

    argv[*argc][ll] = '\0';

    /*logMsg("i1=%d i2=%d >%s<\n",i1,i2,argv[*argc],4,5,6);*/
    (*argc) ++;

    if(flag) k = i2 + 2;
    else     k = i2 + 1;
  }

  return(0);
}



#if defined Linux || defined Darwin

/* Linux and Darwin do not have gethrtime() */

/*
  double d1=0.,d2;
#if defined SOLARIS
  struct timespec to;
  clock_gettime(CLOCK_REALTIME,&to);
  d1 = to.tv_sec;
  d2 = to.tv_nsec;
  d1 = d1 + (d2/1000000000.0);
#elif defined LINUX
  struct timeval to;
  gettimeofday(&to,NULL);
  d1 = to.tv_sec;
  d2 = to.tv_usec;
  d1 = d1 + (d2/1000000.0);
#endif
  printf("%f %s\n",d1,stuff);
*/

hrtime_t/*uint64_t*/
gethrtime(void)
{
  static double firstsecs = 0.;
  uint64_t ret;
  double microsecs;
  double d1=0.,d2;
  struct timeval to;
  gettimeofday(&to, NULL);
  d1 = to.tv_sec;
  if(firstsecs==0) firstsecs=d1;
  d1 = d1 - firstsecs;
  d2 = to.tv_usec;
  /*printf("d1=%f d2=%f\n",d1,d2);*/
  microsecs = d2 + d1*1000000.0;
  /*printf("microsecs=%f\n",microsecs);*/
  ret = microsecs;
  /*printf("ret=%ld\n",ret);*/

  /*
  int nsec_total;
  struct timeval to, abs_time;
  nsec_total = 1000 * to.tv_usec + 1000000000 * to.tv_sec;
  if (nsec_total >= 1000000000) {
    abs_time.tv_nsec = nsec_total - 1000000000;
    abs_time.tv_sec  = deltatime->tv_sec + to.tv_sec + 1;
  }
  else {
    abs_time.tv_nsec = nsec_total;
    abs_time.tv_sec  = deltatime->tv_sec + to.tv_sec;
  }
  */

  return(ret);
}

void
gethrtimetest()
{
  int ii;
  uint64_t jj;

  for(ii=0; ii<10; ii++)
  {
    jj = gethrtime();
    printf("jj=%ld\n",jj);
    sleep(1);
  }
  return;
}

#endif


/* codaExecute: was type-specific, trying to make it generic */

/**************************************************************************/
/******************* RunControl interface function ************************/
/**************************************************************************/

/* following messages must be recognized:
 init
 download
 prestart
 end
 pause
 go
 exit
 status
 */

/* example of tcpClient's command:

     ./tcpClient EB5 'download test_ts2'
*/

int codaInit(char *confname);
int codaDownload(char *confname);
int codaPrestart();
int codaGo();
int codaEnd();
int codaPause();
int codaExit();

/* example

codaDownload("adcecal1")

*/


static int
codaExecute(char *command)
{
  int i, j, len, len1;
  char message[128], confname[128];

  len = strlen(command);

  /* check if command length make sence */
  if(len <= 0 || len >= 128)
  {
    printf("codaExecute: ERROR: len=%d - do nothing\n",len);
    return(1);
  }

  /* check if command contains ascii codes only */
  for(i=0; i<len; i++)
  {
    if( !isascii(command[i]) )
	{
      printf("codaExecute: ERROR: non-ascii code [%d]=%d - do nothing\n",
        i,command[i]);
      return(1);
	}
  }

  /* remove leading spaces */
  for(i=0; i<len; i++)
  {
    j = i;
    if(command[j]!=' ') break;
  }

  strcpy(message,(char *)&command[j]);
  len = strlen(message);
  printf("codaExecute reached, message >%s<, len=%d\n",message,len);





  if( !strncmp(message, "configure", 9) )
  {
    printf("codaExecute: 'configure' transition\n");

    len1 = strlen("configure");
    len = strlen(message);
    for(i=len1; i<len; i++)
    {
      j = i;
      if(message[j]!=' ') break; /* remove leading spaces */
    }
    strcpy(confname,(char *)&message[j]);
    len = strlen(confname);
    printf("--> confname >%s<, len=%d\n",confname,len);

    codaInit(confname);
  }
  else if( !strncmp(message, "download", 8) )
  {
    printf("codaExecute: 'download' transition\n");

    len1 = strlen("download");
    len = strlen(message);
    for(i=len1; i<len; i++)
    {
      j = i;
      if(message[j]!=' ') break; /* remove leading spaces */
    }
    strcpy(confname,(char *)&message[j]);
    len = strlen(confname);
    printf("--> confname >%s<, len=%d\n",confname,len);

    codaDownload(confname);
  }
  else if( !strncmp(message, "prestart", 8) )
  {
    printf("codaExecute: 'prestart' transition\n");
    codaPrestart();
  }
  else if( !strncmp(message, "end", 3) )
  {
    printf("codaExecute: 'end' transition\n");
    codaEnd();
  }
  else if( !strncmp(message, "pause", 5) )
  {
    printf("codaExecute: 'pause' transition\n");
    codaPause();
  }
  else if( !strncmp(message, "go", 2) )
  {
    printf("codaExecute: 'go' transition\n");
    codaGo();
  }
  else if( !strncmp(message, "exit", 4) )
  {
    printf("codaExecute: 'exit' transition\n");
    codaExit();
  }
  else if( !strncmp(message, "alive", 5) )
  {
    printf("codaExecute: 'alive' request - do nothing for now\n");
    /*UDP_standard_request(localobject->name,localobject->state);*/
  }
  else
  {
    printf("codaExecute: ERROR: unknown request >%s<\n",message);
  }
  printf("codaExecute done\n");

  return(0);
}


/* to handle 128-bit words, needed by event building process */

#define MY_INT_BIT 32

static void
Print32(unsigned int k)
{
  int i, lastbit;
  uint32_t j, jj;

  j = 1;
  lastbit = 8*sizeof(int) - 1;
  for(i = 0; i < lastbit; i++) j *= 2;

  jj = j;
  for(i = lastbit; i >=0; i--)
  {
    if(k & jj) fprintf(stdout, "|");
    else       fprintf(stdout, "o");
    
    jj = jj >> 1;
  }

  return;
}

void
Print128(WORD128 *hw)
{
  int i;
  uint32_t *w;
  w = (uint32_t *)hw->words;
  printf("128> ");
  for(i=3; i>=0; i--) {Print32(w[i]); printf(" ");}
  printf("\n");

  return;
}

void
String128(WORD128 *hw, char *str, int len)
{
  int i;
  uint32_t *w;
  w = (uint32_t *)hw->words;
  if(len>50)
  {
    sprintf(str,"0x%08x 0x%08x 0x%08x 0x%08x",w[3],w[2],w[1],w[0]);
  }
  else
  {
    printf("ERROR in String128: str too short (len=%d) - exit\n",len);
  }

  return;
}

void
Copy128(WORD128 *hws, WORD128 *hwd)
{
  int i;
  uint32_t *a = (uint32_t *)hws->words;
  uint32_t *b = (uint32_t *)hwd->words;
  for(i=0; i<4; i++) b[i] = a[i];
  return;
}

void
AND128(WORD128 *hwa, WORD128 *hwb, WORD128 *hwc)
{
  int i;
  uint32_t *a = (uint32_t *)hwa->words;
  uint32_t *b = (uint32_t *)hwb->words;
  uint32_t *c = (uint32_t *)hwc->words;
  for(i=0; i<4; i++) c[i] = a[i] & b[i];
  return;
}

void
OR128(WORD128 *hwa, WORD128 *hwb, WORD128 *hwc)
{
  int i;
  uint32_t *a = (uint32_t *)hwa->words;
  uint32_t *b = (uint32_t *)hwb->words;
  uint32_t *c = (uint32_t *)hwc->words;
  for(i=0; i<4; i++) c[i] = a[i] | b[i];
  return;
}

void
XOR128(WORD128 *hwa, WORD128 *hwb, WORD128 *hwc)
{
  int i;
  uint32_t *a = (uint32_t *)hwa->words;
  uint32_t *b = (uint32_t *)hwb->words;
  uint32_t *c = (uint32_t *)hwc->words;
  for(i=0; i<4; i++) c[i] = a[i] ^ b[i];
  return;
}

int
CheckBit128(WORD128 *hw, int n)
{
  uint32_t *w = (uint32_t *)hw->words;
  int whatword, whatbit;
  uint32_t mask = 1;

  whatword = n / MY_INT_BIT;
  whatbit = n % MY_INT_BIT;
  mask <<= whatbit;

  return((w[whatword] & mask) == mask);
}

void
SetBit128(WORD128 *hw, int n)
{
  uint32_t *w = (uint32_t *)hw->words;
  int whatword, whatbit;
  uint32_t mask = 1;

  whatword = n / MY_INT_BIT;
  whatbit = n % MY_INT_BIT;
  mask <<= whatbit;
  w[whatword] |= mask;

  return;
}

int
EQ128(WORD128 *hwa, WORD128 *hwb)
{
  int i;
  uint32_t *a = (uint32_t *)hwa->words;
  uint32_t *b = (uint32_t *)hwb->words;
  for(i=0; i<4; i++)
  {
	/*printf("i=%d->%d %d\n",i,a[i],b[i]);*/
    if(a[i]!=b[i]) return(0);
  }
  return(1);
}

int
IFZERO128(WORD128 *hwa)
{
  int i;
  uint32_t *a = (uint32_t *)hwa->words;
  for(i=0; i<4; i++)
  {
    if(a[i]!=0) return(0);
  }
  return(1);
}

void
Clear128(WORD128 *hw)
{
  memset(hw->words,0,sizeof(WORD128));/* dol'she?*/

  return;
}

void
Negate128(WORD128 *hw)
{
  int i;
  uint32_t *w;
  w = hw->words;
  for(i=0; i<4; i++) w[i] = ~w[i];
  return;
}
