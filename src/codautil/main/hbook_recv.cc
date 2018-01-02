
/* hbook_recv.cc */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


using namespace std;
#include <strstream>
#include <fstream>
#include <iomanip>

#include <string>
#include <iostream>

#include "hbook.h"

#include "ipc_lib.h"
#include "MessageActionHist.h"

IpcServer &server = IpcServer::Instance();

int
main()
{
  int debug = 1;
  int done = 0;

  // connect to ipc server
  server.init(getenv("EXPID"), getenv("SESSION"), "daq", "hist", "daq", "hist");
  //server.init(NULL, NULL, "*", "*", NULL, "*");

  MessageActionHist *hist = new MessageActionHist((char *)"hbook_recv",debug);
  server.addActionListener(hist);

  while(done==0)
  {
    sleep(1);
  }

}
