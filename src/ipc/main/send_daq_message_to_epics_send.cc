
/* msg_to_epics_server_send.cc */

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

#define MAXELEM 10

int
main()
{
  int32_t  iarray[MAXELEM] = {-1,-2,-3,-4,-5,-6,-7,-8,-9,-19};
  uint32_t uarray[MAXELEM] = {10,9,8,7,6,5,4,3,2,1};
  float    farray[MAXELEM] = {0.0,1.1,3.3,4.4,5.5,6.6,7.7,8.8,9.9,10.99};
  double   darray[MAXELEM] = {0.0,2.277,3.3333,4.455555,5.5333333,6.66666666,7.7888888888,8.83333333,9.92222222,10.991111111};
  uint8_t  carray[MAXELEM] = {18,19,33,66,77,25,25,25,27,28};
  iarray[0] = 12345678;

  /* params: (expid,session,myname,chname,chtype,nelem,data_array) */
  /*
  send_daq_message_to_epics("clasrun","clasprod","daq", "TestScalers", "int", MAXELEM, iarray);
  send_daq_message_to_epics("clasrun","clasprod","daq", "EventRate", "int", 1, iarray);
  send_daq_message_to_epics("clasrun","clasprod","daq", "TestVals", "float", MAXELEM, farray);
  send_daq_message_to_epics("clasrun","clasprod","daq", "DoubleVals", "double", MAXELEM, darray);
  */


  epics_json_msg_sender_init("clasrun","clasprod","daq","HallB_DAQ");

  while(1)
  {
	//printf("1\n");
    epics_json_msg_send("TestScalers", "int", MAXELEM, iarray);
    epics_json_msg_send("EventRate", "int", 1, iarray);
    epics_json_msg_send("TestVals", "float", MAXELEM, farray);
    epics_json_msg_send("DoubleVals", "double", MAXELEM, darray);
  }

  epics_json_msg_close();

}

