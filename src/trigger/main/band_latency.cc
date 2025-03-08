/* band_latency.c - set latency to band's v1495 */

#include <stdio.h>
#include <stdlib.h>

#include "BANDTriggerBoardRegs.h"

//#include "guiband.h"

#include "cratemsgclient.h"
#include "libtcp.h"
#include "libdb.h"

CrateMsgClient *tcp; //sergey: global for now, will find appropriate place later

static char *hostname = (char *)"adcband1";

int
main(int argc, char **argv)
{
  unsigned int ret, tmp;
  unsigned int trigger_latency = 1170;

  // if(argc != 2)
  // {
  //   fprintf(stderr, "error: have to specify hostname\n");
  //   return(1);     
  // }

  //printf("Trying to connect to >%s<\n",argv[1]);

  printf("Connect reached\n");fflush(stdout);

  tcp = new CrateMsgClient(hostname,6102);
  if(tcp->IsValid())
  {
    printf("Connected\n");

	unsigned short sval = 0xFBFB;
	ret = tcp->Read16((unsigned int)(BAND_BOARD_ADDRESS_1+0x800C), &sval);
    printf("ret=%d, VME FIRMWARE val=0x%04x\n",ret,sval);
				  
	unsigned int val = 0xFBFBFBFB;
	ret = tcp->Read32((unsigned int)(BAND_BOARD_ADDRESS_1+0x1000), &val);
    printf("ret=%d, USER FIRMWARE val=0x%08x\n",ret,val);
  }
  else
  {
    printf("NOT CONNECTED - EXIT\n");
    exit(0);
  }

  
  tmp = trigger_latency;
  printf("writing trigger_latency=%u\n",tmp);
  tcp->Write32(BAND_BOARD_ADDRESS_1 + BAND_TRIG0_LATENCY, &tmp);
  

  tcp->Read32(BAND_BOARD_ADDRESS_1 + BAND_TRIG0_LATENCY, &tmp);
  printf("reading trigger_latency=%u\n",tmp);

  exit(0);
}
