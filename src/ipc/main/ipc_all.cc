
/* ipc_all.cc */

#define USE_ACTIVEMQ

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


using namespace std;
#include <strstream>
#include <fstream>
#include <iomanip>

#include <string>
#include <iostream>

#ifndef Linux_armv7l

#include "ipc_lib.h"
#include "MessageActionDAQ2EPICS.h"
#include "../../../clon/src/dbutil/dbutil.s/MessageActionRUNLOG.h"

IpcServer &server = IpcServer::Instance();

#endif

int
main()
{
  int debug = 1;
  int done = 0;

#ifndef Linux_armv7l

  printf(" use IPC_HOST >%s<\n",getenv("IPC_HOST"));

  // connect to ipc server
  server.AddSendTopic(NULL, NULL, NULL, "HallB_DAQ");

  //server.AddRecvTopic("*", "*", "daq", "*");
  server.AddRecvTopic("*", "*", "runlog", "*");

  //server.AddRecvTopic(getenv("EXPID"), getenv("SESSION"), "daq", "*");
  server.Open();

  //MessageActionDAQ2EPICS *epics = new MessageActionDAQ2EPICS();
  //epics->set_debug(debug);
  //server.AddCallback(epics);

  MessageActionJSON       *json = new MessageActionJSON(1);
  //json->set_debug(debug);
  server.AddCallback(json);

  while(done==0)
  {
    sleep(1);
  }

  server.Close();

#endif

}
