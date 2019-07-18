
/* hbook_send.cc */

#include <stdio.h>

#include "hbook.h"

#include "ipc_lib.h"
IpcServer &server = IpcServer::Instance();

char *app  = getenv("EXPID");
char *dest = NULL;

int
main()
{
  // connect to ipc server
  server.AddSendTopic(getenv("EXPID"), getenv("SESSION"), (char *)"daq", (char *)"hist");
  server.AddRecvTopic(getenv("EXPID"), getenv("SESSION"), (char *)"daq", (char *)"hist");
  server.Open();

  Hbook hbook;

  hbook.hbook1(1, (char *)"clondaq3:test1", 100, 0.0, 100.0);
  hbook.hreset(1, (char *)"clondaq3:test1");
  for(int i=0; i<100; i++)
  {
    hbook.hfill(1, ((float)i), 0.0, ((float)i)*2./2.);
  }
  hbook.hprint(1);

  hbook.hist2ipc(1, (char *)"hbook_send");

  server.Close();
}
