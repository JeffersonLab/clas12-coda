
/* tcpServer.c - TCP server by Sergey Boyarinov, last revision May 2011 */ 
/* DESCRIPTION This file contains the server-side of the VxWorks TCP example code. 
   The example code demonstrates the usage of several BSD 4.4-style socket routine calls. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <dlfcn.h>

#ifdef Linux
#include <sys/prctl.h>
#endif

#include "libtcp.h" 
#include "libdb.h" 

#ifdef __cplusplus
typedef int 		(*FPTR) (...);     /* ptr to function returning int */
typedef void 		(*VOIDFPTR) (...); /* ptr to function returning void */
#else
typedef int 		(*FPTR) ();	   /* ptr to function returning int */
typedef void 		(*VOIDFPTR) (); /* ptr to function returning void */
#endif			/* _cplusplus */


#undef DEBUG

/* readiness flag */
static int request_in_progress;

/* task flag */
static int iTaskTCP;

/* function declarations */ 
extern char *targetName(); /* from roc_component.c */

static void tcpServerWorkTask(TWORK *targ); 
static int TcpServer(void);


/* currently processed message */
static char current_message[REQUEST_MSG_SIZE];
static char mysql_host[128];
static char localname[128];

/*
the maximum number of parameters is 'NARGS', currently 8, extra parameters will be ignored;
'NARGS' can be bigger, hopefully 8 will be enough

'my_execute' accepts strings in "" quotes, 0x, floats and ints; it treats single quotes ''
as string, not as a character; since it treats '' as string, to send a single character
use its digital code, for example instead of 'A' use 65

examples:

tcpClient tage "test3(0x55,66.336,44)"
(prints: test3: a=85, b=66.335999, c=44)

if want to send a string:

tcpClient tage 'test4(0x55,66.3366666,"sss",44)'
 or
tcpClient tage "test4(0x55,66.3366666,'sss',44)"
(printf: test4: a=85, b=66.336670, c=>sss<, d=44)

*/


#define FUNCALL(A0,A1,A2,A3,A4,A5,A6,A7) \
(*(command_ptr)) (A7##args[0],A6##args[1],A5##args[2],A4##args[3],A3##args[4],A2##args[5],A1##args[6],A0##args[7])

#define NARGS 8

static int
my_execute(char *string)
{
  int ii, len;
  int64_t res;
  char *saveptr;
  char *command, *arguments;
  char *args[NARGS], nargs;
  VOIDFPTR command_ptr;
  char str[256];
  void *handler;
  float fargs[NARGS];
  int *iargs = (int *)fargs;
  char **sargs = (char **)fargs;

  /* parsing*/
  strncpy(str,string,255); /*strtok will modify input string, let it be local and non-constant one*/
#ifdef DEBUG
  printf("my_execute: str >%s<\n",str);
#endif
  command = strtok_r(str,"(",&saveptr);
  if(command!=NULL)
  {
#ifdef DEBUG
    printf("command >%s<\n",command);
#endif
  }
  else
  {
    printf("no command found in >%s<\n",str);
    return(-1);
  }

  arguments = strtok_r(NULL,")",&saveptr);
  if(arguments!=NULL)
  {
#ifdef DEBUG
    printf("arguments >%s<\n",arguments);
#endif
    args[0] = strtok_r(arguments,",",&saveptr);
    nargs = 1;

    while( (nargs<NARGS) && (args[nargs]=strtok_r(NULL,",",&saveptr)) != NULL ) nargs ++;

    for(ii=0; ii<nargs; ii++)
    {
      if( (strchr(args[ii],'"')!=NULL) || (strchr(args[ii],'\'')!=NULL) ) /*string*/
      {
        sargs[ii] = args[ii];
        while(sargs[ii][0]==' ') sargs[ii] ++; /*remove leading spaces*/
        len = strlen(sargs[ii]);
        while(sargs[ii][len-1]==' ') len--; /*remove trailing spaces*/
        sargs[ii][len] = '\0';
#ifdef DEBUG
        printf("111: sargs[%2d] >%s<\n",ii,sargs[ii]);
#endif
        sargs[ii] ++; /* remove leading quote */
        len = strlen(sargs[ii]);
        sargs[ii][len-1] = '\0'; /* remove trailing quote */
#ifdef DEBUG
        printf("222: sargs[%2d] >%s<\n",ii,sargs[ii]);
#endif
	  }
      else if(strchr(args[ii],'.')!=NULL) /*float*/
      {
        sscanf(args[ii],"%f",&fargs[ii]);
#ifdef DEBUG
        printf("flo: args[%2d] >%s< %f\n",ii,args[ii],fargs[ii]);
#endif
	  }
      else if(strchr(args[ii],'x')!=NULL) /*hex*/
      {
        sscanf(args[ii],"%x",&iargs[ii]);
#ifdef DEBUG
        printf("hex: args[%2d] >%s< %d (0x%x)\n",ii,args[ii],iargs[ii],iargs[ii]);
#endif
	  }
	  else /*decimal*/
	  {
        sscanf(args[ii],"%i",&iargs[ii]);
#ifdef DEBUG
        printf("dec: args[%2d] >%s< %d\n",ii,args[ii],iargs[ii]);
#endif
	  }
    }
  }

  /* open symbol table */
  handler = dlopen(NULL, RTLD_NOW | RTLD_GLOBAL);
  if(handler == 0)
  {
	printf("my_execute ERROR: dlopen failed on >%s<\n",dlerror());
    return(-1);
  }

  /* find symbol */
  command = strtok_r(command," ",&saveptr); /*remove leading and trailing spaces if any*/
#ifdef DEBUG
  printf("command1 >%s<\n",command);
#endif
  if(command==NULL)
  {
    printf("no command found in >%s<\n",command);    
    return(-1);
  }
  res = dlsym(handler, command);
  if((res != (-1)) && (res != 0))
  {
#ifdef DEBUG
    printf("INFO: >%s()< routine found\n",command);
#endif
  }
  else
  {
    printf("ERROR: dlsym returned %d\n",res);
    printf("ERROR: >%s()< routine not found\n",command);
    return(-1);
  }
  command_ptr = (VOIDFPTR) res;

#ifdef DEBUG
  printf("ints-> %d(0x%x) %d(0x%x) %d(0x%x) %d(0x%x)\n",
    iargs[0],iargs[0],iargs[1],iargs[1],iargs[2],iargs[2],iargs[3],iargs[3]);
  printf("floats-> %f %f %f %f\n",fargs[0],fargs[1],fargs[2],fargs[3]);
  printf("my_execute: Executing >%s<\n",command);fflush(stdout);
#endif

  FUNCALL(i,i,i,i,i,i,i,i);

#ifdef DEBUG
  printf("my_execute: executed\n");fflush(stdout);
#endif

  /* close symbol table */
  if(dlclose((void *)handler) != 0)
  {
    printf("ERROR: failed to unload >%s<\n",command);
    return(-1);
  }

  return(0);
}


/**************************************************************************** 
* * tcpServer - accept and process requests over a TCP socket 
* * This routine creates a TCP socket, and accepts connections over the socket 
* from clients. Each client connection is handled by spawning a separate 
* task to handle client requests. 
* * This routine may be invoked as follows: 
* -> sp tcpServer
* task spawned: id = 0x3a6f1c, name = t1 
* value = 3829532 = 0x3a6f1c 
* -> MESSAGE FROM CLIENT (Internet Address 150.12.0.10, port 1027): 
* Hello out there 
* * RETURNS: Never, or ERROR if a resources could not be allocated. */ 


int
tcpServer(char *name, char *mysqlhost)
{
  pthread_t id;
  pthread_attr_t attr;
  int status;

  printf("tcpServer reached: name >%s<, mysqlhost >%s<\n",name,mysqlhost);fflush(stdout);
  strcpy(localname,name);
  strcpy(mysql_host,mysqlhost);
  printf("tcpServer reached: localname >%s<, mysql_host >%s<\n",localname,mysql_host);fflush(stdout);

  printf("tcpServer 0\n");fflush(stdout);
  pthread_attr_init(&attr); /* initialize attr with default attributes */
  printf("tcpServer 1\n");fflush(stdout);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  printf("tcpServer 2\n");fflush(stdout);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  printf("tcpServer 3\n");fflush(stdout);
  status = pthread_create(&id, &attr, (void *(*)(void *)) TcpServer, NULL);
  printf("tcpServer 4\n");fflush(stdout);
  if(status!=0)
  {
    printf("tcpServer: ERROR: pthread_create returned %d - exit\n",status);fflush(stdout);
    exit(0); 
  }
  else
  {
    printf("tcpServer: pthread_create succeeded\n");fflush(stdout);
  }
}


#define TRUE  1
#define FALSE 0
#define OK 0
#define ERROR (-1)
#define STATUS int

static int
TcpServer(void)
{ 
  struct sockaddr_in serverAddr; 
  struct sockaddr_in clientAddr; 
  int sockAddrSize;              /* size of socket address structure */ 
  int sFd;                       /* socket file descriptor */ 
  /*int newFd;*/                     /* socket descriptor from accept */ 
  int ix = 0;                    /* counter for work task names */ 
  int portnum = SERVER_PORT_NUM; /* desired port number; can be changed if that number in use enc */
  char workName[16];             /* name of work task */ 
  int on = TRUE;  /* non-blocking */
  int off = FALSE; /* blocking */
  int status;
  static TWORK targ;
  MYSQL *dbsock = NULL;
  MYSQL_RES *result;
  int numRows;
  char tmp[256], *myname, *hname, *ch;
  
  printf("TcpServer(external) reached\n");fflush(stdout);

#ifdef Linux
  prctl(PR_SET_NAME,"tcpServer");
#endif

  /* some cleanup */
  sockAddrSize = sizeof(struct sockaddr_in); 
  memset((char *)&serverAddr, 0, sockAddrSize); 
  memset((char *)&clientAddr, 0, sockAddrSize); 


  /* creates an endpoint for communication and returns a socket file descriptor */
  if((sFd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
  {
    perror("socket"); 
    return(ERROR); 
  } 

  /* set up the local address */ 
  serverAddr.sin_family = AF_INET; 
  serverAddr.sin_port = htons(portnum); 
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* create a TCP-based socket */ 

  /* bind socket to local address */
  while(bind(sFd, (struct sockaddr *)&serverAddr, sockAddrSize) == ERROR)
  {
    printf("TcpServer(external): trying port %d\n",portnum);
    perror("TcpServer(external): bind");

    /* try another port (just increment on one) */
    portnum ++;
    if((portnum-SERVER_PORT_NUM) > 50)
    {
      close(sFd); 
      return(ERROR);
    }

    serverAddr.sin_port = htons(portnum);
  }
  printf("TcpServer(external): bind on port %d\n",portnum);

  /* create queue for client connection requests */ 
  if(listen (sFd, SERVER_MAX_CONNECTIONS) == ERROR)
  {
    perror ("listen"); 
    close (sFd); 
    return (ERROR);
  }

  myname = localname;
  printf("TcpServer(external): myname >%s<\n",myname);fflush(stdout);

  /* update daq database 'Ports' table with port number and host name */
  dbsock = dbConnect(mysql_host, "daq");

  /* trying to select our name from 'Ports' table */
  sprintf(tmp,"SELECT Name FROM Ports WHERE Name='%s'",myname);
  if(mysql_query(dbsock, tmp) != 0)
  {
	printf("mysql error (%s)\n",mysql_error(dbsock));
    return(ERROR);
  }

  /* gets results from previous query */
  /* we assume that numRows=0 if our Name does not exist,
     or numRows=1 if it does exist */
  if( !(result = mysql_store_result(dbsock)) )
  {
    printf("ERROR in mysql_store_result (%)\n",mysql_error(dbsock));
    return(ERROR);
  }
  else
  {
    numRows = mysql_num_rows(result);
    mysql_free_result(result);

	hname = getenv("HOST");
	printf("TcpServer(external): hname befor >%s<\n",hname);
    /* remove everything starting from first dot */
    ch = strstr(hname,".");
    if(ch != NULL) *ch = '\0';


    /*else ch = hname[strlen(hname)];
	printf("TcpServer(external): hname after >%s<\n",hname);
	*/
    printf("TcpServer(external): hname after >%s<\n",ch);


    /*printf("nrow=%d\n",numRows);*/
    if(numRows == 0)
    {
      sprintf(tmp,"INSERT INTO Ports (Name,Host,tcpClient_tcp) VALUES ('%s','%s',%d)",
        myname,hname,portnum);
    }
    else if(numRows == 1)
    {
      sprintf(tmp,"UPDATE Ports SET Host='%s',tcpClient_tcp=%d WHERE Name='%s'",hname,portnum,myname);
    }
    else
    {
      printf("ERROR: unknown nrow=%d",numRows);
      return(ERROR);
    }

    if(mysql_query(dbsock, tmp) != 0)
    {
	  printf("ERROR\n");
      return(ERROR);
    }
    else
    {
      printf("TcpServer(external): Query >%s< succeeded\n",tmp);
    }
  }

  dbDisconnect(dbsock);


  request_in_progress = 0;
  /* accept new connect requests and spawn tasks to process them */ 
  while(1)
  {
    /*printf("inside while loop\n");*/

    /* do not accept new request if current one is not finished yet; too
    many requests may create network buffer shortage */
    if(request_in_progress)
    {
      printf("TcpServer(external): wait: request in progress\n");
      sleep(1);
      continue;
    }

    /*printf("before accept\n");*/
    if((targ.newFd = accept (sFd, (struct sockaddr *) &clientAddr, &sockAddrSize))
          == ERROR)
    {
      perror ("accept"); 
      close (sFd); 
      return (ERROR); 
    }

    /*printf("accepted request, targ.newFd=%d\n",targ.newFd);*/
    targ.address = inet_ntoa(clientAddr.sin_addr);
    targ.port = ntohs (clientAddr.sin_port);

    sprintf (workName, "tTcpWork%d", ix++);
	/*
usrNetStackSysPoolStatus("tcpServer",1);
usrNetStackDataPoolStatus("tcpServer",1);
	*/
    request_in_progress = 1;
    /* spawn with floating point flag VX_FP_TASK, just in case if some code needs it */
    /*printf("TcpServer: start work thread\n");*/
	{
      int ret;
	  pthread_t id;
      pthread_attr_t detached_attr;

      pthread_attr_init(&detached_attr);
      pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
      pthread_attr_setscope(&detached_attr, PTHREAD_SCOPE_SYSTEM);
	  /*
      printf("befor: socket=%d address>%s< port=%d\n",
        targ.newFd, targ.address, targ.port); fflush(stdout);
	  */
      /* block annoying IP address(es) */
      /*if(!strncmp((int) inet_ntoa (clientAddr.sin_addr),"129.57.71.",10))*/

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
        printf("TcpServer(external): WARN: ignore request from %s\n",targ.address);
        close(targ.newFd);
        request_in_progress = 0;
	  }
      else
	  {
        ret = pthread_create(&id, &detached_attr, (void *(*)(void *)) tcpServerWorkTask, &targ);
        if(ret!=0)
        {
          printf("TcpServer(external): ERROR: pthread_create(CODAtcpServerWorkTask) returned %d\n",
            ret);
          close(targ.newFd);
          request_in_progress = 0;
        }
	  }
	}
	/*
usrNetStackSysPoolStatus("tcpServer",2);
usrNetStackDataPoolStatus("tcpServer",2);
	*/

    /* sleep 100 msec before processing next request; we do not want to process
    too many requests per minute to avoid network buffers shortage */
    /*sleep(1):*/
  }

} 

/**************************************************************************** 
* * tcpServerWorkTask - process client requests 
* * This routine reads from the server's socket, and processes client 
* requests. If the client requests a reply message, this routine 
* will send a reply to the client. 
* * RETURNS: N/A. */ 

static void
tcpServerWorkTask(TWORK *targ)
	 /*int sFd, char *address, unsigned short port) */
{
  int ret;
  TREQUEST clientRequest;            /* request/message from client */ 
  int nRead;                               /* number of bytes read */ 
  char message[REQUEST_MSG_SIZE];
  int len, oldstdout;
#ifdef Linux
  prctl(PR_SET_NAME,"tcp_server_work");
#endif

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
    strcpy(current_message, message);

    /* try Executing the message (each component must provide codaExecute() function */
    /*do not print: message may contains bad characters, it will be checked inside codaExecute
           printf("Executing >%s< (len=%d)\n",message,strlen(message));*/


    fflush(stdout);

    oldstdout = dup(STDOUT_FILENO); /*save stdout*/
    dup2(targ->newFd,STDOUT_FILENO); /*redirect stdout*/

	/*close(targ->newFd);*/

	/* check if message makes sence */
    my_execute(message);

	dup2(oldstdout, STDOUT_FILENO); /*restore stdout*/

    ret = close(oldstdout);  /* close server socket connection */ 
    if(ret<0) perror("close oldstdout: ");
  }
  else if(nRead == 0)
  {
    printf("TcpServer(external): connection closed, exit thread\n");
  }
  else
  {
    perror("ERROR (recv)"); 
  }

  /*free(targ->address);-stuck here !!!*/ /* free malloc from inet_ntoa() */ 

  ret = close(targ->newFd);  /* close server socket connection */ 
  if(ret<0) perror("close targ->newFd: ");

  request_in_progress = 0;

  /* terminate calling thread */
  pthread_exit(NULL);

}
