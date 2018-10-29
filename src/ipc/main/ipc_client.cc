
/* ipc_client.cc - ipc client testing program */


#include "ipc_lib.h"

#include "MessageActionControl.h"
#include "MessageActionTest.h"

/* smartsockets - activemq

 application - session ?
 unique_id   - 

.............- topic/queue


*/

IpcServer &server = IpcServer::Instance();

int
main(int argc AMQCPP_UNUSED, char* argv[] AMQCPP_UNUSED)
{
  int i;

    std::cout << "=====================================================\n";
    std::cout << "Starting the example:" << std::endl;
    std::cout << "-----------------------------------------------------\n";

    //============================================================
    // set to true to use topics instead of queues
    // Note in the code above that this causes createTopic or
    // createQueue to be used in both consumer an producer.
    //============================================================
    int numMessages = 1;
    int ret;

    server.AddSendTopic(getenv("EXPID"), NULL, NULL, (char *)"server");

    server.AddRecvTopic(getenv("EXPID"), NULL, NULL, (char *)"server");
    server.AddRecvTopic(getenv("EXPID"), NULL, NULL, (char *)"test");

	printf("ipc_client: Opening server...\n");fflush(stdout);
    server.Open();
	printf("ipc_client: Opened !\n");fflush(stdout);

    MessageActionControl  *control = new MessageActionControl((char *)"server");
    MessageActionTest        *test = new MessageActionTest();

    server.AddCallback(control);
    server.AddCallback(test);

    long long startTime = System::currentTimeMillis();



    for(i=0; i<numMessages; i++)
	{
	  std::string str = "qwerty";

printf("=====================================================================================\n");

printf("0===================================================================================0\n");
      server << clrm << "testtest";
printf("1===================================================================================1\n");
      server << (int32_t)333 << (uint8_t)5 << (float)7.7 << (double)987987987076.99;
printf("2===================================================================================2\n");
      server << str << (const char *)"abcd";
printf("3===================================================================================3\n");

/*
      int array[5]={9, 8, 7, 6, 5};
      int* arrayPtr = &array[0];
      float farray[5]={9.9, 8.8, 7.7, 6.6, 5.5};
      std::vector<double> v({2.5, 3.6, 4.5, 5.5, 6.5});


      server << SetSize(4) << SetOffset(1) << arrayPtr;
printf("4===================================================================================4\n");

      server << SetSize(2) << SetOffset(1) << array;
printf("5===================================================================================5\n");

      server << SetSize(2) << SetOffset(1) << farray;
printf("6===================================================================================6\n");

      server << SetSize(4) << SetOffset(1) << v;
printf("7===================================================================================7\n");


      server << SetSize(4) << SetOffset(1) << arrayPtr;
*/
printf("8===================================================================================8\n");
      server << SetTopic("clasrun.*.*.test");
printf("9===================================================================================9\n");
      server << endm;
printf("10=================================================================================10\n");
	}

    server << clrm << (char *)"control" << "quit" << endm;


    //printf("done1=%d\n",control->done);
    sleep(1);
    //printf("done2=%d\n",control->done);

    long long endTime = System::currentTimeMillis();
    double totalTime = (double)(endTime - startTime) / 1000.0;


    server.Close();


    std::cout << "Time to completion = " << totalTime << " seconds." << std::endl;
    std::cout << "-----------------------------------------------------\n";
    std::cout << "Finished with the client." << std::endl;
    std::cout << "=====================================================\n";

	exit(0);
}

