/*
 ipc.cc
*/

#define ENABLE_RECEIVE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

using namespace std;
#include <strstream>
#include <iomanip>
#include <fstream>
#include <string>
#include <iostream>

#include "ipc.h"

#if 0
#include "json/json.hpp"
using json = nlohmann::json;
#include "epicsutil.h"
#endif

pthread_mutex_t epicsipcMutex = PTHREAD_MUTEX_INITIALIZER;
#define EPICSIPCLOCK    if (pthread_mutex_lock(&epicsipcMutex) < 0) \
                      perror("pthread_mutex_lock");
#define EPICSIPCUNLOCK  if (pthread_mutex_unlock(&epicsipcMutex) < 0) \
                      perror("pthread_mutex_unlock");

int library_initialized_counter = 0; /* see comments in ipc_lib.h */

#include "ipc_lib.h"
#ifdef ENABLE_RECEIVE
#include "MessageActionControl.h"
MessageActionControl *control;
#endif

static IpcServer &server = IpcServer::Instance();
static int do_not_send = 0;

#define STRLEN 128

int
epics_json_msg_sender_init(const char *expid_, const char *session_, const char *send_unique_id_, const char *send_topic_, const char *recv_unique_id_, const char *recv_topic_)
{
  int status = 0;
  pthread_t t1;
  strstream temp;
  char expid[STRLEN+1];
  char session[STRLEN+1];
  char send_unique_id[STRLEN+1];
  char send_topic[STRLEN+1];
  char recv_unique_id[STRLEN+1];
  char recv_topic[STRLEN+1];
  char send_str[STRLEN+1];
  char recv_str[STRLEN+1];
  char *token;
  int len[2], new_request[2];
  char *str[2];

  EPICSIPCLOCK;
  
  printf("use IPC_HOST >%s<\n",getenv("IPC_HOST"));

  if(expid_==NULL || strlen(expid_)==0)
  {
    strcpy(expid,(char*)getenv("EXPID"));
  }
  else
  {
    strncpy(expid,expid_,STRLEN);
  }

  if(session_==NULL || strlen(session_)==0)
  {
    strcpy(session,(char*)getenv("SESSION"));
  }
  else
  {
    strncpy(session,session_,STRLEN);
  }

  if(send_unique_id_==NULL || strlen(send_unique_id_)==0)
  {
    strcpy(send_unique_id,(char*)"epics_json_msg_sender");
  }
  else
  {
    strncpy(send_unique_id,send_unique_id_,STRLEN);
  }

  if(send_topic_==NULL || strlen(send_topic_)==0)
  {
    strcpy(send_topic,(char*)"HallB_DAQ");
  }
  else
  {
    strncpy(send_topic,send_topic_,STRLEN);
  }

  if(recv_unique_id_==NULL || strlen(recv_unique_id_)==0)
  {
    strcpy(recv_unique_id,(char*)"epics_json_msg_sender");
  }
  else
  {
    strncpy(recv_unique_id,recv_unique_id_,STRLEN);
  }

  if(recv_topic_==NULL || strlen(recv_topic_)==0)
  {
    strcpy(recv_topic,(char*)"HallB_DAQ");
  }
  else
  {
    strncpy(recv_topic,recv_topic_,STRLEN);
  }

  //printf("epics_json_msg_sender_init: expid >%s<, session >%s<, send_unique_id >%s<, send_topic >%s<, recv_unique_id >%s<, recv_topic >%s<\n",expid,session,send_unique_id,send_topic,recv_unique_id,recv_topic);

  sprintf(send_str,"%s.%s.%s.%s",expid,session,send_unique_id,send_topic);
  sprintf(recv_str,"%s.%s.%s.%s",expid,session,recv_unique_id,recv_topic);
  printf("\n>>> epics_json_msg_sender_init request: send_str >%s<, recv_str >%s<, server.Inited()=%d\n\n",send_str,recv_str,server.Inited());
  

  /*if server already opened, get existing topics and check if requested ones already there*/

  if(server.Inited())
  {
    len[0] = server.GetSendTopicLength();
    str[0] = strdup(server.GetSendTopicString());
    len[1] = server.GetRecvTopicLength();
    str[1] = strdup(server.GetRecvTopicString());
    printf("EXISTING SERVER(SEND): LEN = %d, STR >%s<\n",len[0],str[0]);
    printf("EXISTING SERVER(RECV): LEN = %d, STR >%s<\n",len[1],str[1]);

    new_request[0] = 1;
    token = strtok(str[0], ",");
    while (token != NULL)
    {
      printf("   token(send) >%s<\n", token);fflush(stdout);
      if(strcmp(send_str, token) == 0)
      {
	new_request[0] = 0;
	printf("      requested send_str already there - do nothing\n");fflush(stdout);
	break;
      }
      token = strtok(NULL, ",");
    }
    free(str[0]);
    
    new_request[1] = 1;
    token = strtok(str[1], ",");
    while (token != NULL)
    {
      printf("   token(recv) >%s<\n", token);fflush(stdout);
      if(strcmp(recv_str, token) == 0)
      {
	new_request[1] = 0;
	printf("      requested recv_str already there - do nothing\n");fflush(stdout);
	break;
      }
      token = strtok(NULL, ",");
    }
    free(str[1]);

    if(new_request[0]>0 || new_request[1]>0)
    {
      printf("Closing server (server.Inited()=%d)\n",server.Inited());fflush(stdout);
      server.Close();
      printf("Server closed\n");fflush(stdout);

      if(new_request[0])
      {
	printf("      will add send_str >%s<\n",send_str);fflush(stdout);
	server.AddSendTopic(expid, session, send_unique_id, send_topic);
      }
      if(new_request[1])
      {
	printf("      will add recv_str >%s<\n",recv_str);fflush(stdout);
	server.AddRecvTopic(expid, session, recv_unique_id, recv_topic);
      }
      
      printf("Opening server\n");fflush(stdout);
      status = server.Open();
      if(status<0)
      {
        printf("epics_json_msg_sender_init: unable to connect to ipc server on host %s\n",getenv("IPC_HOST"));fflush(stdout);
	EPICSIPCUNLOCK;
        return(-1);
      }
      else
      {
        printf("Server opened\n");fflush(stdout);
        printf("RESTARTED SERVER(SEND): LEN = %d, STR >%s<\n",server.GetSendTopicLength(),server.GetSendTopicString());
        printf("RESTARTED SERVER(RECV): LEN = %d, STR >%s<\n",server.GetRecvTopicLength(),server.GetRecvTopicString());
#ifdef ENABLE_RECEIVE
        control = new MessageActionControl((char *)"json_for_daq"); /*ERROR ??? DO IT ONCE ???*/
        control->setDebug(0); // parameter is debug=0/1
        server.AddCallback(control);
#endif
      }

    }
    
  }
  else
  {
    server.AddSendTopic(expid, session, send_unique_id, send_topic);
    server.AddRecvTopic(expid, session, recv_unique_id, recv_topic);

    status = server.Open();
    if(status<0)
    {
      printf("epics_json_msg_sender_init: unable to connect to ipc server on host %s\n",getenv("IPC_HOST"));
      EPICSIPCUNLOCK;
      return(-1);
    }
    else
    {
      printf("NEW SERVER(SEND): LEN = %d, STR >%s<\n",server.GetSendTopicLength(),server.GetSendTopicString());
      printf("NEW SERVER(RECV): LEN = %d, STR >%s<\n",server.GetRecvTopicLength(),server.GetRecvTopicString());

#ifdef ENABLE_RECEIVE
      control = new MessageActionControl((char *)"json_for_daq");
      control->setDebug(0); // parameter is debug=0/1
      server.AddCallback(control);
#endif
    }
  }


  EPICSIPCUNLOCK;
  
  return(0);
}


#define MAXELEM 16384

int
epics_json_msg_send(const char *caname, const char *catype, int nelem, void *data)
{
  strstream message;

  int ii;
  int32_t  *iarray;
  uint32_t *uarray;
  float    *farray;
  double   *darray;
  uint8_t  *carray;

  EPICSIPCLOCK;

  
#ifdef ENABLE_RECEIVE
  if(!strncmp(caname,"STA:",4))
  {
    do_not_send = control->getNostats();
    if(do_not_send==1)
    {
      //printf("---> nostats=%d - do not send 'STA:' message\n",do_not_send);
      EPICSIPCUNLOCK;
      return(0);
    }
  }
#endif


  

  
  /* params check */

  if(caname==NULL)
  {
    printf("epics_json_msg_send: ERROR: caname undefined\n");
    EPICSIPCUNLOCK;
    return(-1);
  }

  if(catype==NULL)
  {
    printf("epics_json_msg_send: ERROR: catype undefined\n");
    EPICSIPCUNLOCK;
    return(-1);
  }

  if(nelem <=0 || nelem>MAXELEM)
  {
    printf("epics_json_msg_send: ERROR: nelem=%d, must be between 1 and MAXELEM\n",nelem);
    EPICSIPCUNLOCK;
    return(-1);
  }

  /* clear message */
  server << clrm;

  /* server << "json"; - json messages do not have format string in front of json */

  /* construct json message manually */
  message << "{" << "\"" << caname << "\"" << ":";
  if(nelem>1) message << "[";
  if( !strcmp(catype,"int"))
  {
    for(ii=0; ii<nelem; ii++)
    {
      message << (int32_t)((int32_t *)data)[ii];
      if(ii<nelem-1) message << ",";
    }
  }
  else if( !strcmp(catype,"uint"))
  {
    for(ii=0; ii<nelem; ii++)
    {
      message << (uint32_t)((uint32_t *)data)[ii];
      if(ii<nelem-1) message << ",";
    }
  }
  else if( !strcmp(catype,"float"))
  {
    for(ii=0; ii<nelem; ii++)
    {
      message << std::fixed << std::setprecision(5) << (float)((float *)data)[ii];
      if(ii<nelem-1) message << ",";
    }
  }
  else if( !strcmp(catype,"double"))
  {
    for(ii=0; ii<nelem; ii++)
    {
      message  << std::fixed << std::setprecision(5) << (double)((double *)data)[ii];
      if(ii<nelem-1) message << ",";
    }
  }
  else if( !strcmp(catype,"uchar"))
  {
    for(ii=0; ii<nelem; ii++)
    {
      message << (uint8_t)((uint8_t *)data)[ii];
      if(ii<nelem-1) message << ",";
    }
  }
  else if( !strcmp(catype,"string"))
  {
    /* array of strings have to be defined as 'char *sdata[1] = {"bit0_short"};',
    'char *sdata[2] = {"blabla1","blabla2"};', etc */
    for(ii=0; ii<nelem; ii++)
    {
      //printf("str >%s<\n", (char *)((char **)data)[ii] );
      message  << std::fixed << std::setprecision(5) << (char *)((char **)data)[ii];
      if(ii<nelem-1) message << ",";
    }
  }
  else
  {
    printf("epics_json_msg_send: ERROR: unknown catype >%s<\n",catype);
    EPICSIPCUNLOCK;
    return(-1);
  }
  if(nelem>1) message << "]";
  message << "}" << ends;


  //cout << "will send >" << message.str() << "<" << endl;
  server << message.str();

  /*end and send message*/
  server << endm;



  /*
  the stream 'message' is now frozen due to str();
  output to a frozen stream may be truncated;
  freeze(false) must be called or the  destructor will leak
  */
  message.freeze(false);


  /*
  int ia[5] = {1,2,3,4,5};
server << clrm << "abckjgfdhgjksfdhgdfjgkljfdklghjsdfkhgjsdf;kljhksl;gjhkl;sgjhkl;sjhklj" << endm;
//server << clrm << ia << endm;
server << clrm << "abckjgfdhgjksfdhgdfjgkljfdklghjsdfkhgjsdf;kljhksl;gjhkl;sgjhkl;sjhklj" << endm;
  */

  EPICSIPCUNLOCK;

  return(0);
}

int
epics_json_msg_close()
{
  int status;

  EPICSIPCLOCK;
  status = server.Close();
  EPICSIPCUNLOCK;

  return(0);
}





/*
to be used by daq
*/

int
send_daq_message_to_epics(const char *caname, const char *catype, int nelem, void *data)
{
  epics_json_msg_send(caname, catype, nelem, data);

  return(0);
}

int
send_control_message(const char *command, const char *option)
{  
  EPICSIPCLOCK;
  //epics_json_msg_send(caname, catype, nelem, data);  
  server << clrm << command << option << endm;  // for example: server << clrm << "command:coda_ebc" << "nostats" << endm;
  EPICSIPCUNLOCK;

  return(0);
}
