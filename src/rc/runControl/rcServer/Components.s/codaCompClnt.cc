
//-----------------------------------------------------------------------------
// Copyright (c) 1994,1995 Southeastern Universities Research Association,
//                         Continuous Electron Beam Accelerator Facility
//
// This software was developed under a United States Government license
// described in the NOTICE file included as part of this distribution.
//
// CEBAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
//       coda@cebaf.gov  Tel: (804) 249-7030     Fax: (804) 249-5800
//-----------------------------------------------------------------------------
//
// Description:
//      CODA remote component communication routines
//
// Author:  
//      Jie Chen
//      CEBAF Data Acquisition Group
//
// Revision History:
//   $Log: codaCompClnt.cc,v $
//   Revision 1.18  1998/11/05 20:11:38  heyes
//   reverse status updating to use UDP, fix other stuff
//
//   Revision 1.17  1998/10/16 15:08:51  heyes
//   order list of components by type
//
//   Revision 1.16  1998/06/02 19:51:44  heyes
//   fixed rcServer
//
//   Revision 1.15  1997/10/24 12:23:18  heyes
//   fix dplite bug
//
//   Revision 1.14  1997/09/19 17:57:20  heyes
//   longer timeouts
//
//   Revision 1.13  1997/09/16 12:28:41  heyes
//   make comms async
//
//   Revision 1.12  1997/09/11 16:07:12  heyes
//   fix DP_cmd
//
//   Revision 1.11  1997/08/25 15:57:30  heyes
//   use dplite.h
//
//   Revision 1.10  1997/06/13 21:30:43  heyes
//   for marki
//
//   Revision 1.9  1997/05/20 15:12:26  heyes
//   Always DP_ask for status
//
//   Revision 1.8  1997/02/25 19:27:19  heyes
//   added zap command
//
//   Revision 1.7  1997/02/03 13:47:31  heyes
//   add ask command
//
//   Revision 1.6  1996/11/27 15:06:09  chen
//   fix CDEV and Set internal itcl component state before doing state transition
//
//   Revision 1.5  1996/10/31 15:56:05  chen
//   Fixing boot stage bug + reorganize code
//
//   Revision 1.4  1996/10/28 20:32:07  heyes
//   timeout on DP_ask
//
//   Revision 1.3  1996/10/28 14:23:01  heyes
//   communication with ROCs changed
//
//   Revision 1.2  1996/10/14 20:02:46  heyes
//   changed message system
//
//   Revision 1.1.1.1  1996/10/11 13:39:19  chen
//   run control source
//
//
#include <stdio.h>
#include <string.h>
#include <daqConst.h>
#include <daqState.h>


/************************************************/
/************************************************/
/* Sergey: tcpClient as DP (dp_ask) replacement */


/* tcpClient.c - TCP client example */ 
/* DESCRIPTION This file contains the client-side of the VxWorks TCP example code. 
   The example code demonstrates the usage of several BSD 4.4-style socket routine calls. */ 


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#define TRUE 1
#define FALSE 0
#define OK 0
#define ERROR -1
#define STD_IN stdin


#define DEBUG_MSGS


#include "libtcp.h"
#include "libdb.h"


#define TIMEOUT_IN_SECONDS 10

static void
alarmHandler(int sig)
{
  printf("codaCompClnt: alarmHandler: timeout (%d sec) exit from tcpClient\n",TIMEOUT_IN_SECONDS);fflush(stdout);
  signal(sig, alarmHandler);
  return;
}

/**************************************************************************** 
* * tcpClient - send requests to server over a TCP socket 
* * This routine connects over a TCP socket to a server, and sends a 
* user-provided message to the server.
* * This routine may be invoked as follows: 
* -> tcpClient("remoteSystemName","This is my message");  
* * RETURNS: OK, or ERROR if the message could not be sent to the server. */ 

static int
tcpClient(char *name, char *message)
{
  struct hostent *hptr;
  TREQUEST myRequest; /* request to send to server */ 
  struct sockaddr_in serverAddr; /* server's socket address */ 
  int sockAddrSize; /* size of socket address structure */ 
  int sFd; /* socket file descriptor */ 
  int mlen; /* length of message */
  int rBytes;
  int i, portnum = 0;
  char hostname[128];
  MYSQL *dbsock;
  char tmp[1000], temp[100];

  /* create client's socket */ 
  if((sFd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
  {
    perror("socket"); 
    return(ERROR); 
  } 

  /* bind not required - port number is dynamic */ 
  /* build server socket address */ 
  sockAddrSize = sizeof (struct sockaddr_in); 
  memset((char *) &serverAddr, 0, sockAddrSize); 
  serverAddr.sin_family = AF_INET; 


  /* get port and host from DB; if no record in DB for <target name> - exit */
  dbsock = dbConnect(getenv("MYSQL_HOST"), getenv("EXPID"));

  /* use 'inuse' field */
  sprintf(tmp,"SELECT inuse FROM process WHERE name='%s'",name);
#ifdef DEBUG_MSGS
  printf("tcpClient: DB select: >%s<\n",tmp);
#endif
  if(dbGetInt(dbsock, tmp, &portnum)==ERROR) return(ERROR);

  if(portnum==0)
  {
    printf("tcpClient: Component %s has zero 'inuse' field in process table, probably not ready\n",name);
    return(ERROR);
  }

  sprintf(tmp,"SELECT host FROM process WHERE name='%s'",name);
#ifdef DEBUG_MSGS
  printf("tcpClient: DB select: >%s<\n",tmp);
#endif
  if(dbGetStr(dbsock, tmp, hostname)==ERROR) return(ERROR);

  dbDisconnect(dbsock);
#ifdef DEBUG_MSGS
  printf("tcpClient: hostnamee=>%s< portnum=%d\n",hostname,portnum);
#endif
  serverAddr.sin_port = htons(portnum);

  hptr = gethostbyname(hostname);
  if(hptr == NULL)
  {
    printf("tcpClient: unknown hostname >%s<\n",hostname); 
    close(sFd);
    exit(1);
  }
  else
  {
    memcpy(&serverAddr.sin_addr,*(hptr->h_addr_list),sizeof(sizeof(struct in_addr)));
  }

  
  signal(SIGALRM,alarmHandler);
  alarm(TIMEOUT_IN_SECONDS); /* in 'TIMEOUT_IN_SECONDS' seconds connect call will be interrupted if did not finished */
  
  printf("tcpClient: calling connect() to host >%s< port %d (name >%s<) ..\n",hostname,portnum,name);fflush(stdout);
  
  /* connect to server */ 
  if(connect (sFd, (struct sockaddr *) &serverAddr, sockAddrSize) != 0)
  {
    perror("connect"); 
    close(sFd); 
    return(ERROR); 
  }
   
  printf("tcpClient: .. connected to >%s<!\n",hostname);fflush(stdout);
  
  alarm(0); /* clear alarm so it will not interrupt us */

  /* build request, prompting user for message */ 

  myRequest.msgLen = strlen(message);
  sprintf(myRequest.message,"%s\0",message);
  myRequest.reply = FALSE;

#ifdef DEBUG_MSGS
  printf("tcpClient: sending %d bytes: %s\n",myRequest.msgLen, myRequest.message);
#endif

  /* convert integers to network byte order */
  myRequest.msgLen = htonl(myRequest.msgLen);
  myRequest.reply = htonl(myRequest.reply);

  /* send request to server */ 
  if(write(sFd, (char *) &myRequest, sizeof (myRequest)) == ERROR)
  {
    perror("write"); 
    close(sFd); 
    return(ERROR);
  } 

  close(sFd); 

  return(OK); 
} 





/***************************************/
/***************************************/
/***************************************/


#include "codaCompClnt.h"

int compTimeOut = 20;  // mseconds

#define   CODA_SCRIPT_LENGTH 256


/* dummy DP initialization */
int
DP_cmd_init(char *)
{
  return(0);
}


/* called at download */
int
codaDaCreate(char *name, int version, int id, char *type, char *host, int interval)
{
  int state = 0;
  int status;
  char temp [1000];
  char res[1000];

#ifdef DEBUG_MSGS
  printf("codaDaCreate reached for %s\n",name);fflush(stdout);
#endif

  // codaDaReport second arg is 1 means "really ask"
  // printf("here in codaDaCreate for %s\n",name);
  if((state = codaDaReport (name, interval)) == CODA_DISCONNECTED)
  {
    return(state);
  }
  else if(state >= CODA_BOOTED)
  {
    status = CODA_SUCCESS;
    if(status != CODA_SUCCESS)
    {
	  printf("%s Failed to start UDP : %s\n",name,res);
      return(CODA_DISCONNECTED);
    }
    return(CODA_BOOTED);
  }
  else
  {
    printf("%s disconnected\n",name );
    return(CODA_DISCONNECTED);
  }

}

int
codaDaConnect(char *name, int version, int id, char *type, char *host)
{
#ifdef DEBUG_MSGS
  printf("codaDaConnect reached\n");
#endif
  exit(0);
  return(CODA_BOOTED);
}

int
codaDaRemove(char *name)
{
#ifdef DEBUG_MSGS
  printf("codaDaRemove reached\n");
#endif
  exit(0);
  return(CODA_DORMANT);
}


 /*sergey*/
int
codaDaBoot(char* name)
{
#ifdef DEBUG_MSGS
  printf("codaDaBoot reached (name >%s<)\n",name);fflush(stdout);
#endif
  /*
  int status = 0;
  char temp [CODA_SCRIPT_LENGTH];

  status = tcpClient(name,"boot");

  return(status);
  */
  return CODA_SUCCESS;
}

/* sergey: was never called, using to send Configure transition command */
int
codaDaConfigure(char *name, char *param)
{
  int status = 0;
  char temp[CODA_SCRIPT_LENGTH];

#ifdef DEBUG_MSGS
  printf("codaDaConfigure reached\n");fflush(stdout);
#endif

  sprintf(temp,"configure %s",param);
  status = tcpClient(name,temp);

#ifdef DEBUG_MSGS
  printf("CONFCONFCONFCONFCONFCONFCONFCONFCONFCONF\n");
#endif

  if(status == CODA_SUCCESS)
  {
    return(CODA_CONFIGURING);
  }
  else
  {
    return(CODA_DORMANT);
  }
}

int
codaDaDownload(char *name, char *paramL)
{
  int status = 0;
  char temp[CODA_SCRIPT_LENGTH];

#ifdef DEBUG_MSGS
  printf("codaCompClnt::codaDaDownload reached\n");fflush(stdout);
#endif
  sprintf(temp,"download %s",paramL);
  status = tcpClient(name,temp);

#ifdef DEBUG_MSGS
  printf("DOWNLOADDOWNLOADDOWNLOADDOWNLOADDOWNLOAD\n");
#endif

  if(status == CODA_SUCCESS)
  {
    return(CODA_DOWNLOADING);
  }
  else
  {
    return(CODA_DORMANT);
  }
}

int
codaDaPrestart(char *name, int rn, int rt)
{
  int status = 0;
  char temp [CODA_SCRIPT_LENGTH];

  status = tcpClient(name,(char *)"prestart");
  if(status == CODA_SUCCESS)
  {
    return(CODA_PRESTARTING);
  }
  else
  {
    return(CODA_DORMANT);
  }
}

int
codaDaEnd(char *name, int special)
{
  int status = 0;
  char temp [CODA_SCRIPT_LENGTH];

  status = tcpClient(name,(char *)"end");
  if(status == CODA_SUCCESS)
  {
    return(CODA_ENDING);
  }
  else
  {
    return(CODA_DORMANT);
  }
}

int
codaDaPause(char *name, int mask)
{
  int status = 0;
  char temp [CODA_SCRIPT_LENGTH];

  status = tcpClient(name,(char *)"pause");
  if(status == CODA_SUCCESS)
  {
    return(CODA_PAUSING);
  }
  else
  {
    return(CODA_DORMANT);
  }
}

int
codaDaGo(char *name, int mask)
{
  int status = 0;
  char temp [CODA_SCRIPT_LENGTH];

#ifdef DEBUG_MSGS
  printf("codaDaGo reached, name >%s<, mask=0x%08x\n",name,mask);
#endif

  status = tcpClient(name,(char *)"go");
  if(status == CODA_SUCCESS)
  {
    return(CODA_ACTIVATING);
  }
  else
  {
    return(CODA_DORMANT);
  }
}

int
codaAskComponent(char *command, char **res)
{
  int status = 0;
  char temp [CODA_SCRIPT_LENGTH];
  static char result[1000];;
  *res = result;

  strcpy(temp,command);
#ifdef DEBUG_MSGS
  printf("codaAskComponent >%s<\n",temp);
#endif
  exit(0);
  strtok(temp," ");

  printf("codaAskComponent: do not know howto ..\n");

  return(status);
}


int
codaDaReset(char *name)
{
  int status = 0;
  char temp [CODA_SCRIPT_LENGTH];

  status = tcpClient(name,(char *)"exit");
      
  return(status);
}
    
int
codaDaTerminate(char *name)
{
#ifdef DEBUG_MSGS
  printf("codaDaTerminate reached\n");
#endif
  exit(0);

  return(codaDaReset(name));
}

int
codaDaZap(char *name)
{
  int status = 0;
  char temp [CODA_SCRIPT_LENGTH];

#ifdef DEBUG_MSGS
  printf("codaDaZap reached\n");
#endif
  exit(0);

  sprintf (temp, "dp_after 1000 %s zap", name);
  printf("codaDaZap: do not know howto ..\n");
  return(status);
}

int
codaDaReport(char *name, int how)
{
  int status = 0;
  char temp [CODA_SCRIPT_LENGTH];
  char res[1000];
  char res2[1000];
  MYSQL *dbsock;
  char tmp[1000];

#ifdef DEBUG_MSGS
  printf("codaDaReport reached for %s\n",name);
#endif
  /* get component status from DB */
  dbsock = dbConnect(getenv("MYSQL_HOST"), getenv("EXPID"));
  sprintf(tmp,"SELECT state FROM process WHERE name='%s'",name);
#ifdef DEBUG_MSGS
  printf("codaDaReport: DB select: >%s<\n",tmp);
#endif
  if(dbGetStr(dbsock, tmp, res)==ERROR) status = CODA_ERROR;
  else                                  status = CODA_SUCCESS;
  dbDisconnect(dbsock);

#ifdef DEBUG_MSGS
  printf("codaDaReport: name>%s< state>%s<\n",name,res);
#endif
  if(status == CODA_SUCCESS)
  {
/*
    sprintf (temp, "set_udpRate %d000",how);
#ifdef DEBUG_MSGS
    printf("codaDaReport, set rate to %d ",how);
#endif
    status = DP_cmd(name,temp,res2,compTimeOut);
#ifdef DEBUG_MSGS
    printf(" Seconds , cmd result is %s\n",res2);
#endif
*/
    if (::strcmp (res, "dormant") == 0)          return CODA_DORMANT;
    else if (::strcmp (res, "booting") == 0)     return CODA_BOOTING;
    else if (::strcmp (res, "booted") == 0)      return CODA_BOOTED;
    else if (::strcmp (res, "configuring") == 0) return CODA_CONFIGURING;
    else if (::strcmp (res, "configured") == 0)  return CODA_CONFIGURED;
    else if (::strcmp (res, "downloading") == 0) return CODA_DOWNLOADING;
    else if (::strcmp (res, "downloaded") == 0)  return CODA_DOWNLOADED;
    else if (::strcmp (res, "prestarting") == 0) return CODA_PRESTARTING;
    else if (::strcmp (res, "paused") == 0)      return CODA_PAUSED;
    else if (::strcmp (res, "activating") == 0)  return CODA_ACTIVATING;
    else if (::strcmp (res, "active") == 0)      return CODA_ACTIVE;
    else if (::strcmp (res, "ending") == 0)      return CODA_ENDING;
    else if (::strcmp (res, "prestarted") == 0)  return CODA_PRESTARTED;
    else if (::strcmp (res, "resetting") == 0)   return CODA_RESETTING;
  }
  else if (status == -5)
  {
    return CODA_BUSY;
  }
  else
  {
    return CODA_DISCONNECTED;
  }
  return CODA_DISCONNECTED; /*sergey*/
}

int
codaDaReport2 (char* name)
{
  int status = 0;
  char temp [CODA_SCRIPT_LENGTH];
  char res[1000];

#ifdef DEBUG_MSGS
  printf("codaDaReport2 reached\n");
#endif
  exit(0);

  sprintf (temp, "%s status", name);
  printf("codaDaReport: do not know howto .. (2)\n");
  printf("codaDaReport2 status %d result %s\n",status);

  if (status == CODA_SUCCESS) {
    if (::strcmp (res, "dormant") == 0) {
      return CODA_DORMANT;
    }
    else if (::strcmp (res, "booting") == 0) {
      return CODA_BOOTING;
    }
    else if (::strcmp (res, "booted") == 0) {
      return CODA_BOOTED;
    }
    else if (::strcmp (res, "configuring") == 0) {
      return CODA_CONFIGURING;
    }
    else if (::strcmp (res, "configured") == 0) {
      return CODA_CONFIGURED;
    }
    else if (::strcmp (res, "downloading") == 0) {
      return CODA_DOWNLOADING;
    }
    else if (::strcmp (res, "downloaded") == 0) {
      return CODA_DOWNLOADED;
    }
    else if (::strcmp (res, "prestarting") == 0) {
      return CODA_PRESTARTING;
    }
    else if (::strcmp (res, "paused") == 0) {
      return CODA_PAUSED;
    }
    else if (::strcmp (res, "activating") == 0) {
      return CODA_ACTIVATING;
    }
    else if (::strcmp (res, "active") == 0) {
      return CODA_ACTIVE;
    }
    else if (::strcmp (res, "ending") == 0) {
      return CODA_ENDING;
    }
    else if (::strcmp (res, "prestarted") == 0) {
      return CODA_PRESTARTED;
    }
    else if (::strcmp (res, "resetting") == 0) {
      return CODA_RESETTING;
    }
  }
  else if (status == -5) {
    return CODA_BUSY;
  } else {
    return CODA_DISCONNECTED;
  }
}

int
codaDaReadInt(char *name, char *attr, long *value)
{
  int status = 0;
  char temp [CODA_SCRIPT_LENGTH];
  char res[1000];

printf("codaDaReadInt reached\n");
exit(0);
  
  sprintf (temp, "%s configure -%s", 
	    name, attr);

  printf ("%s configure -%s\n", 
	    name, attr);

  printf("codaDaReadInt: do not know howto ..\n");

  if (status == CODA_SUCCESS) {
    ::strncpy (temp, res, sizeof (temp));
    char token0[64], token1[64];
    int  result;
    if (::sscanf (temp, "%s %s %d", token0, token1, &result) >= 3) 
      *value = result;
    else
      *value = 0;
  }
  else
    *value = 0;

  return status;
}


int
codaDaWriteInt (char* name, char* attr, long* value)
{
  return CODA_SUCCESS;
}

int
codaCreateDaComp (char* name)
{
  return CODA_SUCCESS;
}

int
codaDeleteDaComp (char* name)
{
  return CODA_SUCCESS;
}


int
codaDaCompSetState (char* name, int st)
{
  //sergey: it called, do not know why ...
#ifdef DEBUG_MSGS
  printf("Sergey: DUMMY codaDaCompSetState reached\n");
#endif
  return CODA_SUCCESS;
}



/* called from netComponent::configure() */
int
codaDaCompConfigure (char* name)
{
#ifdef DEBUG_MSGS
  printf("codaCompClnt::codaDaCompConfigure reached (name >%s<)\n",name);fflush(stdout);
#endif


  /* sergey: check components status here - have to wait for 'configured' ??? */

  /*printf("sleep 10 sec\n");*/
  /*sleep(10);*/
  /*printf("slept\n");*/

  return CODA_SUCCESS;
}


/* sergey: if just want to check tcp connection to component */
int
codaDaCheckConnection(char *name)
{
  int status = 0;
  char temp[CODA_SCRIPT_LENGTH];

  sprintf(temp,"alive %s",name);
  status = tcpClient(name,temp);
      
  return(status);
}
