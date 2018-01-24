
/* uthbook_test.cc */

#include <stdio.h>

#include "uthbook.h"

#include "ipc_lib.h"
IpcServer &server = IpcServer::Instance();

char *app  = getenv("EXPID");
char *dest = NULL;

int
main()
{
  // connect to ipc server
  server.AddSendTopic(getenv("EXPID"), getenv("SESSION"), "daq", "hist");
  server.AddRecvTopic(getenv("EXPID"), getenv("SESSION"), "daq", "hist");
  server.Open();

  
  uthbook1(1, "clonpc0:test1", 100, 0.0, 100.0);
  for(int i=0; i<100; i++)
  {
    uthfill(1, ((float)i), 0.0, ((float)i)*2.);
  }
  /*uthprint(1);*/
  uthist2ipc(1, "uthbook_send");
  
  
  uthbook2(2, "clonpc0:test2", 100, 0.0, 100.0, 100, 0.0, 100.0);
  for(int i=0; i<100; i++)
  {
    for(int j=0; j<100; j++)
    {
      uthfill(2, ((float)i), ((float)j), ((float)i)*2.);
    }
  }
  

  uthist2ipc(2, "uthbook_send");

  server.Close();
}
