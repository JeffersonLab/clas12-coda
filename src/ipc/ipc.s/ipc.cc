/*
 ipc.cc
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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

int library_initialized_counter = 0; /* see comments in ipc_lib.h */

#include "ipc_lib.h"
static IpcServer &server = IpcServer::Instance();

#define STRLEN 128

int
epics_json_msg_sender_init(const char *expid_, const char *session_, const char *unique_id_, const char *topic_)
{
  int status = 0;
  pthread_t t1;
  strstream temp;
  char expid[STRLEN+1];
  char session[STRLEN+1];
  char unique_id[STRLEN+1];
  char topic[STRLEN+1];

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

  if(unique_id_==NULL || strlen(unique_id_)==0)
  {
    strcpy(unique_id,(char*)"epics_json_msg_sender");
  }
  else
  {
    strncpy(unique_id,unique_id_,STRLEN);
  }

  if(topic_==NULL || strlen(topic_)==0)
  {
    strcpy(topic,(char*)"HallB_DAQ");
  }
  else
  {
    strncpy(topic,topic_,STRLEN);
  }

  printf("epics_json_msg_sender_init: expid >%s<, session >%s<, unique_id >%s<, topic >%s<\n",expid,session,unique_id,topic);

  server.AddSendTopic(expid, session, unique_id, topic);
  server.AddRecvTopic(expid, session, NULL, (char *)"WHATEVER");
  status = server.Open();
  if(status<0)
  {
    printf("epics_json_msg_sender_init: unable to connect to ipc server on host %s\n",getenv("IPC_HOST"));
    return(-1);
  }

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
  

  /* params check */

  if(caname==NULL)
  {
    printf("epics_json_msg_send: ERROR: caname undefined\n");
    return(-1);
  }

  if(catype==NULL)
  {
    printf("epics_json_msg_send: ERROR: catype undefined\n");
    return(-1);
  }

  if(nelem <=0 || nelem>MAXELEM)
  {
    printf("epics_json_msg_send: ERROR: nelem=%d, must be between 1 and MAXELEM\n",nelem);
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

  return(0);
}

int
epics_json_msg_close()
{
  int status;

  status = server.Close();

  return(0);
}



/*to be used by daq*/
int
send_daq_message_to_epics(const char *expid, const char *session, const char *myname, const char *caname, const char *catype, int nelem, void *data)
{
  epics_json_msg_sender_init(expid, session, myname, "HallB_DAQ");
  epics_json_msg_send(caname, catype, nelem, data);
  epics_json_msg_close();

  return(0);
}






#if 0
  json j3;

  if( !strcmp(catype,"int"))         { iarray = (int32_t)calloc(nelem, sifeof(int)); for(ii=0; ii<nelem; ii++) iarray[ii] = (int32_t)((int *)data)[ii]; j3 = {{caname, iarray}}; free(iarray); }
  else if( !strcmp(catype,"uint"))   { for(ii=0; ii<nelem; ii++) uarray[ii] = (int32_t)((int *)data)[ii]; j3 = {{caname, uarray}}; }
  else if( !strcmp(catype,"float"))  { for(ii=0; ii<nelem; ii++) farray[ii] = (float)((float *)data)[ii]; j3 = {{caname, farray}}; }
  else if( !strcmp(catype,"double")) { for(ii=0; ii<nelem; ii++) darray[ii] = (double)((double *)data)[ii]; j3 = {{caname, darray}}; }
  else if( !strcmp(catype,"uchar"))  { for(ii=0; ii<nelem; ii++) carray[ii] = (char)((char *)data)[ii]; j3 = {{caname, carray}}; }
  /*else if( !strcmp(catype,"string")) for(ii=0; ii<nelem; ii++)  = (char *)(((char **)data)[ii]);*/
  else
  {
    printf("epics_json_msg_send: ERROR: unknown catype >%s<\n",catype);
    return(-1);
  }

  /*regular message - not in use any more
  if( !strcmp(catype,"int"))         for(ii=0; ii<nelem; ii++) server << (int32_t)((int *)data)[ii];
  else if( !strcmp(catype,"uint"))   for(ii=0; ii<nelem; ii++) server << (int32_t)((int *)data)[ii];
  else if( !strcmp(catype,"float"))  for(ii=0; ii<nelem; ii++) server << (float)((float *)data)[ii];
  else if( !strcmp(catype,"double")) for(ii=0; ii<nelem; ii++) server << (double)((double *)data)[ii];
  else if( !strcmp(catype,"uchar"))  for(ii=0; ii<nelem; ii++) server << (char)((char *)data)[ii];
  else if( !strcmp(catype,"string")) for(ii=0; ii<nelem; ii++) server << (char *)(((char **)data)[ii]);
  else
  {
    printf("epics_json_msg_send: ERROR: unknown catype >%s<\n",catype);
    return(-1);
  }
  */

  /*form json message*/
  message << j3.dump() << ends;
#endif
