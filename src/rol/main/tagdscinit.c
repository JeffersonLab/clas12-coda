
/* tagdscinit.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>


#ifdef Linux_vme

#include "jvme.h"
#include "tagdscLib.h"

int
main(int argc, char *argv[])
{
  int res, ndsc, chan;
  char myname[256];
  unsigned int addr, laddr;
  int id = 0; //board id from 0
  unsigned int data, width;

  /* Open the default VME windows */
  vmeOpenDefaultWindows();
  printf("\n");

  
vmeBusLock();

 ndsc = tagdscInit();
 tagdscConfig("");
 tagdscUploadAllPrint();

 for(id=0; id<ndsc; id++)
 {
   printf("\nCSR[%2d] = 0x%08x\n",id,tagdscReadCSR(id));

   printf("Threshold[%2d] = %d\n",id,tagdscReadThreshold(id));
   // tagdscWriteThreshold(id, 30);
   //printf("Threshold[%2d] = %d\n",id,tagdscReadThreshold(id));

   printf("PulseWidth[%2d] = %d\n",id,tagdscReadPulseWidth(id));
   //tagdscWritePulseWidth(id, 50);
   //printf("PulseWidth[%2d] = %d\n",id,tagdscReadPulseWidth(id));

   printf("GateScaler1[%2d] = %d\n",id,tagdscReadGateScaler1(id));
   printf("GateScaler2[%2d] = %d\n",id,tagdscReadGateScaler2(id));

   printf("\n");
   for(chan=0; chan<NTAGDSCCHAN; chan++)
   {
     printf("   ChannelScaler1[%2d][%2d] = %d\n",id,chan,tagdscReadChannelScaler1(id, chan));
   }
   printf("\n");
   for(chan=0; chan<NTAGDSCCHAN; chan++)
   {
     printf("   ChannelScaler2[%2d][%2d] = %d\n",id,chan,tagdscReadChannelScaler2(id, chan));
   }
 }

vmeBusUnlock();


  exit(0);
}

#else

int
main()
{
  return(0);
}

#endif
